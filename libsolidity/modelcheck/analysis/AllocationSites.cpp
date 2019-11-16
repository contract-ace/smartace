/**
 * @date 2019
 * First-pass visitor for converting Solidity ADT's into structs in C.
 */

#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <map>
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

}
}
}
