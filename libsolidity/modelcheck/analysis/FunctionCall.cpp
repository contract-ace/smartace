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

FunctionCallAnalyzer::CallGroup FunctionCallAnalyzer::classify() const
{
	switch (type().kind())
	{
	case FunctionType::Kind::Internal:
	case FunctionType::Kind::External:
	case FunctionType::Kind::BareCall:
	case FunctionType::Kind::BareStaticCall:
		return CallGroup::Method;
	case FunctionType::Kind::DelegateCall:
	case FunctionType::Kind::BareDelegateCall:
	case FunctionType::Kind::BareCallCode:
		return CallGroup::Delegate;
	case FunctionType::Kind::Creation:
		return CallGroup::Constructor;
	case FunctionType::Kind::Send:
		return CallGroup::Send;
	case FunctionType::Kind::Transfer:
		return CallGroup::Transfer;
	case FunctionType::Kind::KECCAK256:
	case FunctionType::Kind::ECRecover:
	case FunctionType::Kind::SHA256:
	case FunctionType::Kind::RIPEMD160:
		return CallGroup::Crypto;
	case FunctionType::Kind::Selfdestruct:
		return CallGroup::Destruct;
	case FunctionType::Kind::Revert:
		return CallGroup::Revert;
	case FunctionType::Kind::Assert:
		return CallGroup::Assert;
	case FunctionType::Kind::Require:
		return CallGroup::Require;
	case FunctionType::Kind::Log0:
	case FunctionType::Kind::Log1:
	case FunctionType::Kind::Log2:
	case FunctionType::Kind::Log3:
	case FunctionType::Kind::Log4:
	case FunctionType::Kind::Event:
		return CallGroup::Logging;
	case FunctionType::Kind::BlockHash:
		return CallGroup::Blockhash;
	case FunctionType::Kind::AddMod:
		return CallGroup::AddMod;
	case FunctionType::Kind::MulMod:
		return CallGroup::MulMod;
	case FunctionType::Kind::ArrayPush:
	case FunctionType::Kind::ByteArrayPush:
		return CallGroup::Push;
	case FunctionType::Kind::ArrayPop:
		return CallGroup::Pop;
	case FunctionType::Kind::ObjectCreation:
		return CallGroup::NewArray;
	case FunctionType::Kind::ABIEncode:
	case FunctionType::Kind::ABIEncodePacked:
	case FunctionType::Kind::ABIEncodeWithSelector:
	case FunctionType::Kind::ABIEncodeWithSignature:
	case FunctionType::Kind::ABIDecode:
		return CallGroup::ABI;
	case FunctionType::Kind::GasLeft:
		return CallGroup::GasLeft;
	case FunctionType::Kind::MetaType:
		return CallGroup::NoOp;
	default:
		return CallGroup::UnhandledCall;
	}
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
