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

NewCallSummary::Children const& NewCallSummary::children() const
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
    return family(_vertex).size();
}

list<NewCallGraph::Label> const& NewCallGraph::family(Label _root) const
{
    auto res = m_family.find(_root);
    if (res == m_family.end())
    {
        throw runtime_error("Data requested on vertex not in NewCallGraph");
    }
    return res->second;
}

NewCallSummary::CallGroup NewCallGraph::violations() const
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
    if (m_family.find(root) != m_family.end()) return;

    list<NewCallGraph::Label> family{root};
    for (auto neighbour : _neighbourhood->second)
    {
        analyze(m_vertices.find(neighbour.type));

        auto const& subtree = m_family[neighbour.type];
        for (unsigned int i = 0; i < neighbour.count; ++i)
        {
            family.insert(family.end(), subtree.begin(), subtree.end());
        }
    }

    m_family[root] = move(family);
}

// -------------------------------------------------------------------------- //

}
}
}
