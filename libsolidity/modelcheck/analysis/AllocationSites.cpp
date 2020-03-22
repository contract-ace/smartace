/**
 * @date 2019
 * First-pass visitor for converting Solidity ADT's into structs in C.
 */

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

NewCallSummary::NewCallSummary(ContractDefinition const& _src)
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
            for (auto const& call :visitor.calls)
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

NewCallSummary::CallGroup const& NewCallSummary::children() const
{
    return m_children;
}

NewCallSummary::CallGroup const& NewCallSummary::violations() const
{
    return m_violations;
}

NewCallSummary::Visitor::Visitor(
    FunctionDefinition const* _context, size_t _depth_limit
)
    : M_DEPTH_LIMIT(_depth_limit)
    , m_context(_context)
{
    if (M_DEPTH_LIMIT == 0)
    {
        throw runtime_error("AllocationSite analysis exceeded depth limit.");
    }
    _context->accept(*this);
}

bool NewCallSummary::Visitor::visit(FunctionCall const& _node)
{
    auto const* NEWTYPE = dynamic_cast<ContractType const*>(
        _node.annotation().type
    );
    auto const* CALLTYPE = dynamic_cast<FunctionType const*>(
        _node.expression().annotation().type
    );

    if (NEWTYPE)
    {
        calls.emplace_back();
        calls.back().callsite = &_node;
        calls.back().context = m_context;
        calls.back().is_retval = m_return;

        // Splits returns from assignments. Returns have implicit destinations.
        if (m_return)
        {
            calls.back().dest = m_context->returnParameters()[0].get();
        }
        else
        {
            calls.back().dest = m_dest;
        }

        // Recursive type analysis for internal calls.
        if (CALLTYPE->kind() == FunctionType::Kind::Creation)
        {
            calls.back().type = (&NEWTYPE->contractDefinition());
        }
        else
        {
            calls.back().type = handle_call_type(*CALLTYPE);
        }

        // Checks that new type analysis succeeded.
        if (!calls.back().type)
        {
            throw runtime_error("AllocationSite failed to resolve new type.");
        }
    }

    for (auto arg : _node.arguments())
    {
        arg->accept(*this);
    }

    return false;
}

bool NewCallSummary::Visitor::visit(Assignment const& _node)
{
    VariableDeclaration const* dest = nullptr;

    auto const* id = LValueSniffer<Identifier>(_node).find();
    if (id)
    {
        dest = dynamic_cast<VariableDeclaration const*>(
            id->annotation().referencedDeclaration
        );
    }

    ScopedSwap<VariableDeclaration const*> scope(m_dest, dest);
    _node.rightHandSide().accept(*this);

    return false;
}

bool NewCallSummary::Visitor::visit(Return const& _node)
{
    ScopedSwap<bool> scope(m_return, true);
    if (auto const* expr = _node.expression())
    {
        expr->accept(*this);
    }
    return false;
}

ContractDefinition const* NewCallSummary::Visitor::handle_call_type(
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

void NewCallGraph::record(SourceUnit const& _src)
{
    // Enforces record is called in the unfinalized state.
    if (m_finalized)
    {
        throw runtime_error("NewCallGraph mutated after finalization.");
    }

    // Analyzes children and violations for all contracts.
    auto contracts = ASTNode::filteredNodes<ContractDefinition>(_src.nodes());
    for (auto contract : contracts)
    {
        if (contract->isLibrary() || contract->isInterface()) continue;
    
        NewCallSummary summary(*contract);

        // Accumates violations.
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
            }
            else if (typedata->second == child.type)
            {
                // Two statements initialize the contract as a consistent type.
                m_vertices[contract].push_back(move(child));
            }
            else
            {
                // The contract is initialized as at least two distinct types.
                child.status = NewCallSummary::ViolationType::TypeConfusion;
                m_violations.push_back(move(child));
            }
        }

        // Updates name-to-contract lookup.
        m_names[contract->name()] = contract;
    }
}

void NewCallGraph::finalize()
{
    if (m_finalized) return;
    m_finalized = true;

    for (auto itr = m_vertices.begin(); itr != m_vertices.end(); ++itr)
    {
        analyze(itr);
    }
}

size_t NewCallGraph::cost_of(Label _vertex) const
{
    if (!m_finalized)
    {
        throw runtime_error("NewCallGraph mutated after finalization.");
    }

    auto itr = m_reach.find(_vertex);
    if (itr != m_reach.end()) return itr->second;
    return 0;
}

NewCallSummary::CallGroup const& NewCallGraph::children_of(Label _vertex) const
{
    if (!m_finalized)
    {
        throw runtime_error("NewCallGraph mutated after finalization.");
    }

    auto itr = m_vertices.find(_vertex);
    if (itr == m_vertices.end())
    {
        throw runtime_error("Unable to find vertex.");
    }
    return itr->second;
}

NewCallSummary::CallGroup const& NewCallGraph::violations() const
{
    return m_violations;
}

NewCallGraph::Label NewCallGraph::reverse_name(string _name) const
{
    auto res = m_names.find(_name);
    if (res == m_names.end()) return nullptr;
    return res->second;
}

void NewCallGraph::analyze(NewCallGraph::Graph::iterator _neighbourhood)
{
    NewCallGraph::Label root = _neighbourhood->first;
    if (m_reach.find(root) != m_reach.end()) return;

    size_t cost = 1;
    for (auto neighbour : _neighbourhood->second)
    {
        analyze(m_vertices.find(neighbour.type));
        cost += m_reach[neighbour.type];
    }

    m_reach[root] = cost;
}

ContractDefinition const& NewCallGraph::specialize(
    VariableDeclaration const& _decl
) const
{
    auto itr = m_truetypes.find(&_decl);
    if ((itr == m_truetypes.end()) || (itr->second == nullptr))
    {
        throw runtime_error("Unable to find declaration: " + _decl.name());
    }
    return (*itr->second);
}

// -------------------------------------------------------------------------- //

}
}
}
