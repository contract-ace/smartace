#include <libsolidity/modelcheck/analysis/FunctionCall.h>

#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/Types.h>

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
 : M_CALL(&_call)
{
    m_args = _call.arguments();

    m_type = dynamic_cast<FunctionType const*>(
        _call.expression().annotation().type
    );

    if (m_type && m_type->hasDeclaration())
    {
		m_type->declaration().accept(*this);
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

bool FunctionCallAnalyzer::is_super() const
{
    return (m_root && (m_root->name() == "super"));
}

bool FunctionCallAnalyzer::is_in_library() const
{
	if (m_method_decl)
	{
    	auto scope = dynamic_cast<ContractDefinition const*>(method_decl().scope());
		return (scope && scope->isLibrary());
	}
	return false;
}

bool FunctionCallAnalyzer::is_low_level() const
{
    return m_low_level;
}

bool FunctionCallAnalyzer::context_is_this() const
{
	if (auto id = dynamic_cast<Identifier const*>(m_context))
	{
		return (id->name() == "this");
	}
	return false;
}

bool FunctionCallAnalyzer::is_getter() const
{
	return (m_getter_decl != nullptr);
}

FunctionType const& FunctionCallAnalyzer::type() const
{
    if (!m_type)
    {
		string error = "Function call encountered without type annotations: ";
		error += get_ast_string(M_CALL);
        throw runtime_error(error);
    }
    return (*m_type);
}

FunctionDefinition const& FunctionCallAnalyzer::method_decl() const
{
    if (!m_method_decl)
    {
		string error = "Method encountered without declaration: ";
		error += get_ast_string(M_CALL);
		throw runtime_error(error);
    }
    return (*m_method_decl);
}

VariableDeclaration const& FunctionCallAnalyzer::getter_decl() const
{
    if (!m_getter_decl)
    {
		string error = "Getter encountered without declaration: ";
		error += get_ast_string(M_CALL);
		throw runtime_error(error);
    }
    return (*m_getter_decl);
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
	else if (_node.memberName() == "call")
	{
		m_low_level = true;
        m_context = (&ExpressionCleaner(_node.expression()).clean());
	}
    else if (_node.memberName() == "value")
    {
        m_value = move(m_last);
    }
    else if (!m_context)
    {
        m_context = (&ExpressionCleaner(_node.expression()).clean());

		auto raw_type = m_context->annotation().type;
		if (auto type = dynamic_cast<ContractType const*>(&unwrap(*raw_type)))
		{
			if (type->contractDefinition().isLibrary())
			{
				m_context = nullptr;
			}
		}
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
    m_root = (&_node);
    return false;
}

// -------------------------------------------------------------------------- //

bool FunctionCallAnalyzer::visit(FunctionDefinition const& _node)
{
	m_method_decl = (&_node);
	return false;
}

bool FunctionCallAnalyzer::visit(VariableDeclaration const& _node)
{
	m_getter_decl = (&_node);
	return false;
}

// -------------------------------------------------------------------------- //

}
}
}
