/**
 * @date 2019
 * Utilities to extract the parameters of a function call. This includes gas,
 * value, and context.
 */

#include "libsolidity/modelcheck/analysis/FunctionCall.h"

#include <iostream>

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

    m_type = dynamic_cast<FunctionType const*>(
        _call.expression().annotation().type
    );
	if (!m_type)
	{
		throw runtime_error("Function encountered without type annotations.");
	}

    if (m_type->hasDeclaration())
    {
        m_decl = dynamic_cast<FunctionDefinition const*>(
            &m_type->declaration()
        );
    }

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

FunctionType const& FunctionCallAnalyzer::type() const
{
    return (*m_type);
}

FunctionDefinition const& FunctionCallAnalyzer::decl() const
{
    if (!m_decl)
    {
		throw runtime_error("Function encountered without declaration.");
    }
    return (*m_decl);
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
    else if (!m_context)
    {
        m_context = (&_node.expression());
    }

    _node.expression().accept(*this);
    return false;
}

bool FunctionCallAnalyzer::visit(FunctionCall const& _node)
{
    if (_node.arguments().empty())
    {
        m_last = nullptr;
    }
    else
    {
        m_last = _node.arguments().front();
    }

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
