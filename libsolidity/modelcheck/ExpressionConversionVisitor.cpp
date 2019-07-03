/**
 * @date 2019
 * Utility visitor to convert Solidity expressions into verifiable code.
 */

#include <libsolidity/modelcheck/ExpressionConversionVisitor.h>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ExpressionConversionVisitor::ExpressionConversionVisitor(
	Expression const& _expr,
	TypeTranslator const& _scope,
	VariableScopeResolver const& _decls
): m_expr(&_expr), m_scope(_scope), m_decls(_decls)
{
}

// -------------------------------------------------------------------------- //

void ExpressionConversionVisitor::print(std::ostream& _stream)
{
	m_ostream = &_stream;
	m_expr->accept(*this);
	m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool ExpressionConversionVisitor::visit(EnumValue const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Enum value not yet supported.");
}

bool ExpressionConversionVisitor::visit(ModifierInvocation const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Modifier invocations not yet supported.");
}

bool ExpressionConversionVisitor::visit(Conditional const& _node)
{
	print_subexpression(_node.condition());
	(*m_ostream) << "?";
	print_subexpression(_node.trueExpression());
	(*m_ostream) << ":";
	print_subexpression(_node.falseExpression());
	return false;
}

bool ExpressionConversionVisitor::visit(Assignment const& _node)
{
	print_subexpression(_node.leftHandSide());

	Token op_tok = _node.assignmentOperator();
	switch (op_tok)
	{
	case Token::AssignSar:
		// TODO(scottwe)
		throw runtime_error("Arithmetic right bit-shift not yet supported.");
	case Token::AssignShr:
		// TODO(scottwe)
		throw runtime_error("Logical right bit-shift not yet supported.");
	case Token::Assign:
	case Token::AssignBitOr:
	case Token::AssignBitXor:
	case Token::AssignBitAnd:
	case Token::AssignShl:
	case Token::AssignAdd:
	case Token::AssignSub:
	case Token::AssignMul:
	case Token::AssignDiv:
	case Token::AssignMod:
		(*m_ostream) << TokenTraits::friendlyName(op_tok);
		break;
	default:
		throw runtime_error("Assignment not yet supported.");
	}

	print_subexpression(_node.rightHandSide());

	return false;
}

bool ExpressionConversionVisitor::visit(TupleExpression const& _node)
{
	if (_node.isInlineArray())
	{
		// TODO(scottwe): Support inline arrays.
		throw runtime_error("Inline arrays not yet supported.");
	}
	else if (_node.components().size() > 1)
	{
		// TODO(scottwe): Support multiple return values.
		throw runtime_error("Multivalue tuples not yet supported.");
	}
	else if (!_node.components().empty())
	{
		(_node.components()[0])->accept(*this);
	}
	return false;
}

bool ExpressionConversionVisitor::visit(UnaryOperation const& _node)
{
	if (!_node.isPrefixOperation())
	{
		print_subexpression(_node.subExpression());
	}

	Token op_tok = _node.getOperator();
	switch (op_tok)
	{
	case Token::Not:
	case Token::BitNot:
		(*m_ostream) << "!";
		break;
	case Token::Delete:
		// TODO(scottwe)
		throw runtime_error("Delete not yet supported.");
	case Token::Inc:
	case Token::Dec:
		(*m_ostream) << TokenTraits::friendlyName(op_tok);
		break;
	default:
		throw runtime_error("UnaryOp not yet supported.");
	}

	if (_node.isPrefixOperation())
	{
		print_subexpression(_node.subExpression());
	}

	return false;
}

bool ExpressionConversionVisitor::visit(BinaryOperation const& _node)
{
	print_subexpression(_node.leftExpression());

	Token op_tok = _node.getOperator();
	switch (op_tok)
	{
	case Token::SAR:
		// TODO(scottwe)
		throw runtime_error("Arithmetic right bit-shift not yet supported.");
	case Token::SHR:
		// TODO(scottwe)
		throw runtime_error("Logical right bit-shift not yet supported.");
	case Token::Exp:
		// TODO(scottwe)
		throw runtime_error("Exponentiation not yet supported.");
	case Token::Comma:
	case Token::Or:
	case Token::And:
	case Token::BitOr:
	case Token::BitXor:
	case Token::BitAnd:
	case Token::SHL:
	case Token::Add:
	case Token::Sub:
	case Token::Mul:
	case Token::Div:
	case Token::Mod:
	case Token::Equal:
	case Token::NotEqual:
	case Token::LessThan:
	case Token::GreaterThan:
	case Token::LessThanOrEqual:
	case Token::GreaterThanOrEqual:
		(*m_ostream) << TokenTraits::friendlyName(op_tok);
		break;
	default:
		throw runtime_error("BinOp not yet supported.");
	}

	print_subexpression(_node.rightExpression());

	return false;
}

bool ExpressionConversionVisitor::visit(FunctionCall const& _node)
{
	auto ftype = dynamic_cast<FunctionType const*>(
		_node.expression().annotation().type);
	if (!ftype)
	{
		throw runtime_error("Function encountered without type annotations.");
	}

	switch (ftype->kind())
	{
	case FunctionType::Kind::Internal:
		// TODO(scottwe): implement.
		throw runtime_error("Internal function call not yet supported.");
	case FunctionType::Kind::External:
	case FunctionType::Kind::BareCall:
	case FunctionType::Kind::BareStaticCall:
		// TODO(scottwe): STATICCALL prevents immutability, as opposed to
		//     relying on only compile-time checks... should this be handled
		//     seperately, and where did compile-time checks fail?
		// TODO(scottwe): how does value behave when chaining calls?
		// TODO(scottwe): implement.
		throw runtime_error("External function call not yet supported.");
	case FunctionType::Kind::DelegateCall:
	case FunctionType::Kind::BareDelegateCall:
	case FunctionType::Kind::BareCallCode:
		// TODO(scottwe): report that calls to DELEGATECALL are not supported.
		throw runtime_error("Delegate calls are unsupported.");
	case FunctionType::Kind::Creation:
		// TODO(scottwe): what functions map to CREATE type?
		throw runtime_error("CREATE call not yet supported.");
	case FunctionType::Kind::Send:
	case FunctionType::Kind::Transfer:
		// TODO(scottwe): associate address and call at MemberAccess level?
		throw runtime_error("Payment call not yet supported.");
	case FunctionType::Kind::KECCAK256:
		// TODO(scottwe): implement.
		throw runtime_error("KECCAK256 not yet supported.");
	case FunctionType::Kind::Selfdestruct:
		// TODO(scottwe): when should this be acceptable?
		throw runtime_error("Selfdestruct unsupported.");
	case FunctionType::Kind::Revert:
		// TODO(scottwe): decide on rollback versus assert branch pruning.
		throw runtime_error("Revert not yet supported.");
	case FunctionType::Kind::ECRecover:
		// TODO(scottwe): implement.
		throw runtime_error("ECRecover not yet supported.");
	case FunctionType::Kind::SHA256:
		// TODO(scottwe): implement.
		throw runtime_error("SHA256 not yet supported.");
	case FunctionType::Kind::RIPEMD160:
		// TODO(scottwe): implement.
		throw runtime_error("RIPEMD160 not yet supported.");
	case FunctionType::Kind::Log0:
	case FunctionType::Kind::Log1:
	case FunctionType::Kind::Log2:
	case FunctionType::Kind::Log3:
	case FunctionType::Kind::Log4:
	case FunctionType::Kind::Event:
		// TODO(scottwe): prune statements which operate on events...
		throw runtime_error("Logging is not verified.");
	case FunctionType::Kind::SetGas:
		// TODO(scottwe): will gas be modelled at all?
		throw runtime_error("`gas(<val>)` not yet supported.");
	case FunctionType::Kind::SetValue:
		// TODO(scottwe): update state.value for the given call.
		throw runtime_error("`value(<val>)` not yet supported.");
	case FunctionType::Kind::BlockHash:
		// TODO(scottwe): implement.
		throw runtime_error("`block.blockhash(<val>)` not yet supported.");
	case FunctionType::Kind::AddMod:
		// TODO(scottwe): overflow free `assert(z > 0); return (x + y) % z;`.
		throw runtime_error("AddMod not yet supported.");
	case FunctionType::Kind::MulMod:
		// TODO(scottwe): overflow free `assert(z > 0); return (x * y) % z;`.
		throw runtime_error("AddMod not yet supported.");
	case FunctionType::Kind::ArrayPush:
	case FunctionType::Kind::ByteArrayPush:
		// TODO(scottwe): implement.
		throw runtime_error("`<array>.push(<val>)` not yet supported.");
	case FunctionType::Kind::ArrayPop:
		// TODO(scottwe): implement.
		throw runtime_error("`<array>.pop()` not yet supported.");
	case FunctionType::Kind::ObjectCreation:
		// TODO(scottwe): implement.
		throw runtime_error("`new <array>` not yet supported.");
	case FunctionType::Kind::Assert:
		print_assertion("assert", _node);
		break;
	case FunctionType::Kind::Require:
		print_assertion("assume", _node);
		break;
	case FunctionType::Kind::ABIEncode:
		// TODO(scottwe): decide how/if this should be used.
		throw runtime_error("`abi.encode(...)` unsupported.");
	case FunctionType::Kind::ABIEncodePacked:
		// TODO(scottwe): decide how/if this should be used.
		throw runtime_error("`abi.encodePacked(...)` unsupported.");
	case FunctionType::Kind::ABIEncodeWithSelector:
		// TODO(scottwe): decide how/if this should be used.
		throw runtime_error("`abi.encodeWithSelector(...)` unsupported.");
	case FunctionType::Kind::ABIEncodeWithSignature:
		// TODO(scottwe): decide how/if this should be used.
		throw runtime_error("`abi.encodeWithSignature(...)` unsupported.");
	case FunctionType::Kind::ABIDecode:
		// TODO(scottwe): decide how/if this should be used.
		throw runtime_error("`abi.decode(...)` unsupported.");
	case FunctionType::Kind::GasLeft:
		// TODO(scottwe): decide how to handle remaining gas checks.
		throw runtime_error("GasLeft not yet supported.");
	case FunctionType::Kind::MetaType:
		// Note: Compiler does not generate code for MetaType calls.
		break;
	default:
		throw runtime_error("Unexpected function call type.");
	}

	return false;
}

bool ExpressionConversionVisitor::visit(NewExpression const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("New expressions not yet supported.");
}

bool ExpressionConversionVisitor::visit(MemberAccess const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Member access not yet supported.");
}

bool ExpressionConversionVisitor::visit(IndexAccess const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Index access not yet supported.");
}

bool ExpressionConversionVisitor::visit(Identifier const& _node)
{
	(*m_ostream) << m_decls.resolve_identifier(_node);
	return false;
}

bool ExpressionConversionVisitor::visit(ElementaryTypeNameExpression const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Elementary type name expressions not yet supported.");
}

bool ExpressionConversionVisitor::visit(Literal const& _node)
{
	switch (_node.token())
	{
	case Token::TrueLiteral:
		(*m_ostream) << "1";
		break;
	case Token::FalseLiteral:
		(*m_ostream) << "0";
		break;
	case Token::Number:
		(*m_ostream) << literal_to_number(_node);
		break;
	case Token::StringLiteral:
		(*m_ostream) << m_hasher(_node.value());
		break;
	default:
		throw runtime_error("Literal type derived from unsupported token.");
	}
	return false;
}

// -------------------------------------------------------------------------- //

long long int ExpressionConversionVisitor::literal_to_number(Literal const& _node)
{
	long long int num;

	istringstream iss(_node.value());
	iss >> num;

	switch(_node.subDenomination())
	{
	case Literal::SubDenomination::Szabo:
		num *= 1000000000000;
		break;
	case Literal::SubDenomination::Finney:
		num *= 1000000000000000;
		break;
	case Literal::SubDenomination::Ether:
		num *= 1000000000000000000;
		break;
	case Literal::SubDenomination::Minute:
		num *= 60;
		break;
	case Literal::SubDenomination::Hour:
		num *= 60 * 60;
		break;
	case Literal::SubDenomination::Day:
		num *= 60 * 60 * 24;
		break;
	case Literal::SubDenomination::Week:
		num *= 60 * 60 * 24 * 7;
		break;
	case Literal::SubDenomination::Year:
		num *= 60 * 60 * 24 * 365;
		break;
	default:
		break;
	}

	return num;
}

// -------------------------------------------------------------------------- //

void ExpressionConversionVisitor::print_subexpression(Expression const& _node)
{
	(*m_ostream) << "(";
	_node.accept(*this);
	(*m_ostream) << ")";
}

void ExpressionConversionVisitor::print_assertion(
	string type, FunctionCall const& _func)
{
	if (_func.arguments().empty())
	{
		throw runtime_error("Assertion requires condition.");
	}

	(*m_ostream) << type << "(";
	(_func.arguments()[0])->accept(*this);
	(*m_ostream) << ")";
}

// -------------------------------------------------------------------------- //

}
}
}
