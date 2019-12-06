/**
 * @date 2019
 * Utilities to extract the parameters of a function call. This includes gas,
 * value, and context.
 */

#include "libsolidity/modelcheck/analysis/FunctionCall.h"

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

FunctionCallAnalyzer::FunctionCallAnalyzer(FunctionCall const& _call)
{
    m_args = _call.arguments();
    _call.expression().accept(*this);
}

// -------------------------------------------------------------------------- //

vector<ASTPointer<Expression const>> const& FunctionCallAnalyzer::args() const
{
    return m_args;
}

ASTPointer<Expression const> FunctionCallAnalyzer::value() const
{
    return m_value;
}

ASTPointer<Expression const> FunctionCallAnalyzer::gas() const
{
    return m_gas;
}
Expression const* FunctionCallAnalyzer::context() const
{
    return m_context;
}

Identifier const* FunctionCallAnalyzer::id() const
{
    return m_id;
}

bool FunctionCallAnalyzer::is_super() const
{
    return (id() && (id()->name() == "super"));
}

// -------------------------------------------------------------------------- //

bool FunctionCallAnalyzer::visit(MemberAccess const& _node)
{
    if (_node.memberName() == "gas")
    {
        m_gas = move(m_last);
    }
    else if (_node.memberName() == "value")
    {
        m_value = move(m_last);
    }
    else
    {
        m_context = (&_node.expression());
    }

    _node.expression().accept(*this);
    return false;
}

bool FunctionCallAnalyzer::visit(FunctionCall const& _node)
{
    m_last = _node.arguments().front();
    _node.expression().accept(*this);
    return false;
}

bool FunctionCallAnalyzer::visit(Identifier const& _node)
{
    m_id = (&_node);
    return false;
}

// -------------------------------------------------------------------------- //

}
}
}
