#include <libsolidity/modelcheck/analysis/AllocationSites.h>

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

VariableDeclaration const* resolve_to_contract(Expression const& _expr)
{
    if (_expr.annotation().type->category() != Type::Category::Contract)
    {
        return nullptr;
    }

    // TODO(scottwe): support structures.
    if (LValueSniffer<MemberAccess>(_expr).find())
    {
        throw runtime_error("Member access to contracts not yet allowed.");
    }

    if (auto const* id = LValueSniffer<Identifier>(_expr).find())
    {
        return dynamic_cast<VariableDeclaration const*>(
            id->annotation().referencedDeclaration
        );
    }

    return nullptr;
}

// -------------------------------------------------------------------------- //

AllocationSummary::AllocationSummary(ContractDefinition const& _src)
{
    for (auto func : _src.definedFunctions())
    {
        Visitor visitor(func, 2);

        if (func->isConstructor())
        {
            for (auto const& call : visitor.calls)
            {
                if (call.dest && call.dest->isStateVariable())
                {
                    m_children.push_back(call);
                }
                else
                {
                    m_violations.push_back(call);
                }
            }
        }
        else if (func->functionType(false) == nullptr)
        {
            for (auto const& call : visitor.calls)
            {
                if (call.is_retval)
                {

                    m_children.push_back(call);
                }
                else
                {
                    m_violations.push_back(call);
                }
            }
        }
        else
        {
            m_violations.splice(m_violations.end(), visitor.calls);
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
    FunctionDefinition const* _context, size_t _depth_limit
): M_DEPTH_LIMIT(_depth_limit) , m_context(_context)
{
    if (M_DEPTH_LIMIT == 0)
    {
        throw runtime_error("AllocationSite analysis exceeded depth limit.");
    }
    _context->accept(*this);
}

bool AllocationSummary::Visitor::visit(FunctionCall const& _node)
{
    auto const* NEWTYPE = dynamic_cast<ContractType const*>(
        _node.annotation().type
    );
    auto const* CALLTYPE = dynamic_cast<FunctionType const*>(
        _node.expression().annotation().type
    );

    if (NEWTYPE)
    {
        // Recursive type analysis for internal calls.
        ContractDefinition const* newcall_rv_type = nullptr;
        if (CALLTYPE->kind() == FunctionType::Kind::Creation)
        {
            newcall_rv_type = (&NEWTYPE->contractDefinition());
        }
        else
        {
            newcall_rv_type = handle_call_type(*CALLTYPE);
        }

        // If the call lacks a "true" return type, it is not an allocation.
        if (newcall_rv_type)
        {
            calls.emplace_back();
            calls.back().callsite = &_node;
            calls.back().context = m_context;
            calls.back().is_retval = m_return;
            calls.back().type = newcall_rv_type;

            // Splits returns from assignments. Return dests are implicit.
            if (m_return)
            {
                calls.back().dest = m_context->returnParameters()[0].get();
            }
            else
            {
                calls.back().dest = m_dest;
            }
        }
    }

    for (auto arg : _node.arguments())
    {
        arg->accept(*this);
    }

    return false;
}

bool AllocationSummary::Visitor::visit(Assignment const& _node)
{
    auto dest = resolve_to_contract(_node.leftHandSide());

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
    FunctionType const& _ftype
)
{
    auto const* CALL = dynamic_cast<FunctionDefinition const*>(
        &_ftype.declaration()
    );

    // Checks if this method was previously analyzed.
    auto res = _fcache.find(CALL);
    if (res != _fcache.end())
    {
        return res->second;
    }

    // If not, analyzes the method.
    if (CALL)
    {
        Visitor nested(CALL, M_DEPTH_LIMIT - 1);
        for (auto child : nested.calls)
        {
            if (child.is_retval)
            {
                _fcache[CALL] = child.type;
                return child.type;
            }
        }
    }

    // No match.
    return nullptr;
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

ContractDefinition const&
    AllocationGraph::resolve(Expression const& _expr) const
{
    auto dest = resolve_to_contract(_expr);
    if (!dest)
    {
        throw runtime_error("Unable to resolve expression.");
    }

    return specialize(*dest);
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
