#include <libsolidity/modelcheck/analysis/StringLookup.h>

#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

StringLookup::StringLookup(FlatModel const& _model, CallGraph const& _graph)
{
    m_registry[""] = 0;
    m_curr_index = 1;

    for (auto contract : _model.view())
    {
        for (auto decl : contract->state_variables())
        {
            decl->accept(*this);
        }
    }

    for (auto function : _graph.executed_code())
    {
        function->accept(*this);
    }

    for (auto modifier : _graph.applied_modifiers())
    {
        modifier->accept(*this);
    }
}

uint32_t StringLookup::lookup(Literal const& _node) const
{
    // Checks that the value is a string literal.
    if (_node.token() != Token::StringLiteral)
    {
        throw runtime_error("StringLookup::lookup called on non-string.");
    }

    // Attempts to locate value.
    auto res = m_registry.find(_node.value());
    if (res == m_registry.end())
    {
        throw runtime_error("StringLookup::lookup called on unknown literal.");
    }

    // Result.
    return res->second;
}

void StringLookup::endVisit(Literal const& _node)
{
    if (_node.token() == Token::StringLiteral)
    {
        if (m_curr_index <= 0)
        {
            throw runtime_error("StringLookup exceeded maximum entry count.");
        }

        if (m_registry.find(_node.value()) == m_registry.end())
        {
            m_registry[_node.value()] = m_curr_index;
            m_curr_index += 1;
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
