/**
 * @date 2019
 * First-pass visitor for converting Solidity ADT's into structs in C.
 */

#include <libsolidity/modelcheck/analysis/AllocationSites.h>

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
    map<ContractDefinition const*, size_t> allocs;
    for (auto func : _src.definedFunctions())
    {
        Visitor visitor(func);

        for (auto call : visitor.calls)
        {
            if (func->isConstructor())
            {
                allocs[call.type] += 1;
            }
            else
            {
                m_violations.emplace_back(call);
            }
        }
    }

    for (auto alloc : allocs)
    {
        m_children.emplace_back();
        m_children.back().type = alloc.first;
        m_children.back().count = alloc.second;
    }
}

list<NewCallSummary::ChildType> const& NewCallSummary::children() const
{
    return m_children;
}

list<NewCallSummary::NewCall> const& NewCallSummary::violations() const
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
        calls.back().context = m_context;
        calls.back().type = (&NEWTYPE->contractDefinition());
    }

    for (auto arg : _node.arguments())
    {
        arg->accept(*this);
    }

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
        NewCallSummary summary(*contract);
        
        auto violations = summary.violations();
        m_violations.splice(m_violations.end(), violations);

        m_vertices[contract] = summary.children();
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

size_t NewCallGraph::cost_of(ContractDefinition const* _vertex) const
{
    auto res = m_costs.find(_vertex);
    if (res == m_costs.end())
    {
        throw runtime_error("Cost requested on vertex not in NewCallGraph");
    }
    return res->second;
}

NewCallSummary::CallGroup NewCallGraph::violations() const
{
    return m_violations;
}

void NewCallGraph::analyze(NewCallGraph::Graph::iterator _neighbourhood)
{
    if (m_costs.find(_neighbourhood->first) != m_costs.end()) return;

    size_t cost = 1;
    for (auto neighbour : _neighbourhood->second)
    {
        analyze(m_vertices.find(neighbour.type));
        cost += neighbour.count * m_costs[neighbour.type];
    }

    m_costs[_neighbourhood->first] = cost;
}

// -------------------------------------------------------------------------- //

}
}
}
