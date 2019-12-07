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
        Visitor visitor(func);

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

NewCallSummary::Visitor::Visitor(FunctionDefinition const* _context)
: m_context(_context)
{
    _context->accept(*this);
}

bool NewCallSummary::Visitor::visit(FunctionCall const& _node)
{
    auto const* NEWTYPE = dynamic_cast<ContractType const*>(
        _node.annotation().type
    );

    if (NEWTYPE)
    {
        calls.emplace_back();
        calls.back().callsite = &_node;
        calls.back().dest = m_dest;
        calls.back().context = m_context;
        calls.back().type = (&NEWTYPE->contractDefinition());
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
        if (contract->isLibrary()) continue;
    
        NewCallSummary summary(*contract);
        
        auto violations = summary.violations();
        m_violations.splice(m_violations.end(), violations);

        m_vertices[contract] = summary.children();

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

// -------------------------------------------------------------------------- //

}
}
}
