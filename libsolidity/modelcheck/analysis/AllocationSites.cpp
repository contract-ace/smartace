#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/General.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

AllocationSummary::AllocationSummary(ContractDefinition const& _src)
{
    for (auto func : _src.definedFunctions())
    {
        Visitor visitor(_src, *func, 2);

        if (func->isConstructor())
        {
            for (auto const& call : visitor.calls)
            {
                if (call.dest && !call.dest->isLocalOrReturn() && call.type)
                {
                    m_children.push_back(call);
                }
                else if (call.type)
                {
                    m_violations.push_back(call);
                    m_violations.back().status = ViolationType::Orphaned;
                }
            }
        }
        else if (func->functionType(false) == nullptr)
        {
            for (auto const& call : visitor.calls)
            {
                if (call.is_retval && call.type)
                {
                    m_children.push_back(call);
                }
                else if (call.type)
                {
                    m_violations.push_back(call);
                    m_violations.back().status = ViolationType::Orphaned;
                }
            }
        }
        else
        {
            for (auto const & call : visitor.calls)
            {
                if (call.type)
                {
                    m_violations.push_back(call);
                    m_violations.back().status = ViolationType::Unbounded;
                }
            }
        }
    }
}

AllocationSummary::CallGroup AllocationSummary::children() const
{
    return m_children;
}

AllocationSummary::CallGroup AllocationSummary::violations() const
{
    return m_violations;
}

AllocationSummary::Visitor::Visitor(
    ContractDefinition const& _src,
    FunctionDefinition const& _context,
    size_t _depth_limit
): M_DEPTH_LIMIT(_depth_limit), m_context(_context), m_src(_src)
{
    if (M_DEPTH_LIMIT == 0)
    {
        throw runtime_error("AllocationSite analysis exceeded depth limit.");
    }
    _context.accept(*this);
}

bool AllocationSummary::Visitor::visit(FunctionCall const& _node)
{
    auto type = _node.annotation().type;
    auto contract = dynamic_cast<ContractType const*>(type);

	if (_node.annotation().kind == FunctionCallKind::FunctionCall)
    {
        FunctionCallAnalyzer call(_node);
        if (contract)
        {
            // Determines the allocation contract.
            ContractDefinition const* def = nullptr;
            if (call.classify() == FunctionCallAnalyzer::CallGroup::Constructor)
            {
                // Case: New call.
                def = (&contract->contractDefinition());
            }
            else if (call.is_super() || call.is_in_library() || !call.context())
            {
                // Case: Internal method call.
                auto match = (&call.decl());
                if (!call.is_super() && !call.is_in_library())
                {
                    string const& name = call.decl().name();
                    match = find_named_match<FunctionDefinition>(&m_src, name);
                }
                def = handle_call_type(*match);
            }
            else if (m_dest)
            {
                throw runtime_error("Allocations restricted to internal calls");
            }

            // Records the allocation.
            calls.emplace_back();
            calls.back().callsite = (&_node);
            calls.back().context = (&m_context);
            calls.back().is_retval = m_return;
            calls.back().type = def;
            if (m_return)
            {
                calls.back().dest = m_context.returnParameters()[0].get();
            }
            else
            {
                calls.back().dest = m_dest;
            }
        }
        if (call.context()) call.context()->accept(*this);
    }

    for (auto arg : _node.arguments())
    {
        arg->accept(*this);
    }

    return false;
}

bool AllocationSummary::Visitor::visit(Assignment const& _node)
{
    VariableDeclaration const* dest = nullptr;
    if (_node.annotation().type->category() == Type::Category::Contract)
    {
        dest = expr_to_decl(_node.leftHandSide());
    }

    ScopedSwap<VariableDeclaration const*> scope(m_dest, dest);
    _node.rightHandSide().accept(*this);

    return false;
}

bool AllocationSummary::Visitor::visit(Return const& _node)
{
    ScopedSwap<bool> scope(m_return, true);
    if (auto const* expr = _node.expression())
    {
        expr->accept(*this);
    }
    return false;
}

ContractDefinition const* AllocationSummary::Visitor::handle_call_type(
    FunctionDefinition const& _call
)
{
    if (_fcache.find(&_call) == _fcache.end())
    {
        _fcache[&_call] = nullptr;
        Visitor nested(m_src, _call, M_DEPTH_LIMIT - 1);
        for (auto child : nested.calls)
        {
            if (child.is_retval)
            {
                _fcache[&_call] = child.type;
                return child.type;
            }
        }
    }
    return _fcache[&_call];
}

// -------------------------------------------------------------------------- //

AllocationGraph::AllocationGraph(vector<ContractDefinition const*> _model)
{
    // Analyzes the model, while building out its graph.
    set<ContractDefinition const*> visited;
    for (size_t i = 0; i < _model.size(); ++i)
    {
        // Ensures this is a new contract.
        auto contract = _model[i];
        if (!visited.insert(contract).second) continue;

        // Runs analysis.
        AllocationSummary summary(*contract);

        // Accumulates violations.
        auto violations = summary.violations();
        m_violations.splice(m_violations.end(), violations);

        // Checks that each contract variable has only one possible type.
        m_vertices[contract] = {};
        for (auto child : summary.children())
        {
            auto typedata = m_truetypes.find(child.dest);
            if (typedata == m_truetypes.end())
            {
                // Caches the first "true type" for the contract.
                m_truetypes[child.dest] = child.type;
                m_vertices[contract].push_back(move(child));
                _model.push_back(child.type);
            }
            else if (typedata->second == child.type)
            {
                // Two statements initialize the contract as a consistent type.
                m_vertices[contract].push_back(move(child));
            }
            else
            {
                // The contract is initialized as at least two distinct types.
                child.status = AllocationSummary::ViolationType::TypeConfusion;
                m_violations.push_back(move(child));
            }
        }
    }

    // Computes cost of each vertex.
    for (auto itr = m_vertices.begin(); itr != m_vertices.end(); ++itr)
    {
        analyze(itr->first, itr->second);
    }
}

size_t AllocationGraph::cost_of(Label _vertex) const
{
    auto itr = m_reach.find(_vertex);
    if (itr != m_reach.end()) return itr->second;
    return 0;
}

AllocationSummary::CallGroup AllocationGraph::children_of(Label _vertex) const
{
    auto itr = m_vertices.find(_vertex);
    if (itr == m_vertices.end())
    {
        throw runtime_error("Unable to find contract: " + _vertex->name());
    }
    return itr->second;
}

AllocationSummary::CallGroup AllocationGraph::violations() const
{
    return m_violations;
}

bool AllocationGraph::retval_is_allocated(VariableDeclaration const& _var) const
{
    auto itr = m_truetypes.find(&_var);
    return ((itr != m_truetypes.end()) && (itr->second != nullptr));
}

ContractDefinition const&
    AllocationGraph::specialize(VariableDeclaration const& _var) const
{
    auto itr = m_truetypes.find(&_var);
    if ((itr == m_truetypes.end()) || (itr->second == nullptr))
    {
        throw runtime_error("Unable to find declaration: " + _var.name());
    }
    return (*itr->second);
}

void AllocationGraph::analyze(
    Label _root, AllocationSummary::CallGroup _children
)
{
    if (m_reach.find(_root) != m_reach.end()) return;

    size_t cost = 1;
    for (auto child : _children)
    {
        auto grandchildren = m_vertices.find(child.type)->second;
        analyze(child.type, grandchildren);
        cost += m_reach[child.type];
    }

    m_reach[_root] = cost;
}

// -------------------------------------------------------------------------- //

}
}
}
