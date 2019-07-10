/**
 * @date 2019
 * Utility visitor to convert Solidity expressions into verifiable code.
 */

#include <libsolidity/modelcheck/ExpressionConverter.h>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

map<pair<MagicType::Kind, string>, string> const 
	ExpressionConverter::m_magic_members{{
	{{MagicType::Kind::Block, "coinbase"}, ""},
	{{MagicType::Kind::Block, "difficulty"}, ""},
	{{MagicType::Kind::Block, "gaslimit"}, ""},
	{{MagicType::Kind::Block, "number"}, "state->blocknum"},
	{{MagicType::Kind::Block, "timestamp"}, "state->blocknum"},
	{{MagicType::Kind::Message, "data"}, ""},
	{{MagicType::Kind::Message, "gas"}, ""},
	{{MagicType::Kind::Message, "sender"}, "state->sender"},
	{{MagicType::Kind::Message, "sig"}, ""},
	{{MagicType::Kind::Message, "value"}, "state->value"},
	{{MagicType::Kind::Transaction, "gasprice"}, ""},
	{{MagicType::Kind::Transaction, "origin"}, ""}
}};

// -------------------------------------------------------------------------- //

ExpressionConverter::ExpressionConverter(
	Expression const& _expr,
	TypeConverter const& _converter,
	VariableScopeResolver const& _decls
): m_expr(&_expr), m_converter(_converter), m_decls(_decls)
{
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print(ostream& _stream)
{
	m_ostream = &_stream;
	m_expr->accept(*this);
	m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool ExpressionConverter::visit(EnumValue const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Enum value not yet supported.");
}

bool ExpressionConverter::visit(ModifierInvocation const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Modifier invocations not yet supported.");
}

bool ExpressionConverter::visit(Conditional const& _node)
{
	print_subexpression(_node.condition());
	(*m_ostream) << "?";
	print_subexpression(_node.trueExpression());
	(*m_ostream) << ":";
	print_subexpression(_node.falseExpression());
	return false;
}

bool ExpressionConverter::visit(Assignment const& _node)
{
	auto map = LValueSniffer<IndexAccess>(_node.leftHandSide()).find();

	bool entry_lval = m_lval;
	m_lval = true;
	if (map)
	{
		(*m_ostream) << "Write_" << m_converter.translate(*map).name << "(";
		print_map_idx_pair(*map);
		(*m_ostream) << ", ";
	}
	else
	{
		print_subexpression(_node.leftHandSide());
		(*m_ostream) << "=";
	}
	m_lval = entry_lval;

	if (_node.assignmentOperator() != Token::Assign)
	{
		(*m_ostream) << "(";
		print_binary_op(
			_node.leftHandSide(),
			TokenTraits::AssignmentToBinaryOp(_node.assignmentOperator()),
			_node.rightHandSide());
		(*m_ostream) << ")";
	}
	else
	{
		print_subexpression(_node.rightHandSide());
	}

	if (map)
	{
		(*m_ostream) << ")";
	}

	return false;
}

bool ExpressionConverter::visit(TupleExpression const& _node)
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

bool ExpressionConverter::visit(UnaryOperation const& _node)
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

bool ExpressionConverter::visit(BinaryOperation const& _node)
{
	print_binary_op(
		_node.leftExpression(),
		_node.getOperator(),
		_node.rightExpression());
	return false;
}

bool ExpressionConverter::visit(FunctionCall const& _node)
{
	switch (_node.annotation().kind)
	{
	case FunctionCallKind::FunctionCall:
		print_function(_node.expression(), _node.arguments());
		break;
	case FunctionCallKind::TypeConversion:
		throw runtime_error("Type conversion not yet supported.");
	case FunctionCallKind::StructConstructorCall:
		print_struct_consructor(_node.expression(), _node.arguments());
		break;
	default:
		throw runtime_error("FunctionCall encountered of unknown kind.");
	}
	return false;
}

bool ExpressionConverter::visit(NewExpression const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("New expressions not yet supported.");
}

bool ExpressionConverter::visit(MemberAccess const& _node)
{
	auto expr_type = _node.expression().annotation().type;

	switch (expr_type->category())
	{
	case Type::Category::Address:
		print_address_member(_node.expression(), _node.memberName());
		break;
	case Type::Category::StringLiteral:
	case Type::Category::Array:
	case Type::Category::FixedBytes:
		print_array_member(_node.expression(), _node.memberName());
		break;
	case Type::Category::Contract:
	case Type::Category::Struct:
		print_adt_member(_node.expression(), _node.memberName());
		break;
	case Type::Category::Magic:
		print_magic_member(expr_type, _node.memberName());
		break;
	default:
		throw runtime_error("MemberAccess applied to invalid type.");
	}

	return false;
}

bool ExpressionConverter::visit(IndexAccess const& _node)
{
	switch (_node.baseExpression().annotation().type->category())
	{
	case Type::Category::Mapping:
		{
			if (m_index_depth)
			{
				(*m_ostream) << "Ref_";
			}
			else if (m_lval)
			{
				(*m_ostream) << "*Ref_";
			}
			else
			{
				(*m_ostream) << "Read_";
			}
			(*m_ostream) << m_converter.translate(_node).name << "(";
			print_map_idx_pair(_node);
			(*m_ostream) << ")";
		}
		break;
	default:
		throw runtime_error("IndexAccess applied to unsupported type.");
	}
	return false;
}

bool ExpressionConverter::visit(Identifier const& _node)
{
	(*m_ostream) << m_decls.resolve_identifier(_node);
	return false;
}

bool ExpressionConverter::visit(ElementaryTypeNameExpression const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Elementary type name expressions not yet supported.");
}

bool ExpressionConverter::visit(Literal const& _node)
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

long long int ExpressionConverter::literal_to_number(Literal const& _node)
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

void ExpressionConverter::print_subexpression(Expression const& _node)
{
	(*m_ostream) << "(";
	_node.accept(*this);
	(*m_ostream) << ")";
}

void ExpressionConverter::print_binary_op(
	Expression const& _lhs, Token _op, Expression const& _rhs
)
{
	print_subexpression(_lhs);

	switch (_op)
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
		(*m_ostream) << TokenTraits::friendlyName(_op);
		break;
	default:
		throw runtime_error("BinOp not yet supported.");
	}

	print_subexpression(_rhs);
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print_map_idx_pair(IndexAccess const& _map)
{
	++m_index_depth;
	if (NodeSniffer<IndexAccess>(_map.baseExpression()).find())
	{
		_map.baseExpression().accept(*this);
	}
	else
	{
		(*m_ostream) << "&";
		print_subexpression(_map.baseExpression());
	}
	--m_index_depth;
	(*m_ostream) << ", ";
	_map.indexExpression()->accept(*this);
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print_struct_consructor(
	Expression const& _struct,
	vector<ASTPointer<Expression const>> const& _args
)
{
	if (auto struct_id = NodeSniffer<Identifier>(_struct).find())
	{
		auto translation = m_converter.translate(*struct_id);
		(*m_ostream) << "Init_" << translation.name << "(";
		for (unsigned int i = 0; i < _args.size(); ++i)
		{
			if (i > 0)
			{
				(*m_ostream) << ", ";
			}
			_args[i]->accept(*this);
		}
		(*m_ostream) << ")";
	}
	else
	{
		throw runtime_error("Struct constructor called without identifier.");
	}
}

void ExpressionConverter::print_function(
	Expression const& _call, vector<ASTPointer<Expression const>> const& _args
)
{
	auto ftype = dynamic_cast<FunctionType const*>(_call.annotation().type);
	if (!ftype)
	{
		throw runtime_error("Function encountered without type annotations.");
	}

	switch (ftype->kind())
	{
	case FunctionType::Kind::Internal:
		print_method(*ftype, nullptr, _args);
		break;
	case FunctionType::Kind::External:
	case FunctionType::Kind::BareCall:
	case FunctionType::Kind::BareStaticCall:
		// TODO(scottwe): STATICCALL prevents immutability, as opposed to
		//     relying on only compile-time checks... should this be handled
		//     seperately, and where did compile-time checks fail?
		// TODO(scottwe): how does value behave when chaining calls?
		print_ext_method(*ftype, _call, _args);
		break;
	case FunctionType::Kind::DelegateCall:
	case FunctionType::Kind::BareDelegateCall:
	case FunctionType::Kind::BareCallCode:
		// TODO(scottwe): report that calls to DELEGATECALL are not supported.
		throw runtime_error("Delegate calls are unsupported.");
	case FunctionType::Kind::Creation:
		print_contract_ctor(_call, _args);
		break;
	case FunctionType::Kind::Send:
	case FunctionType::Kind::Transfer:
		print_payment(_call, _args);
		break;
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
		print_assertion("assert", _args);
		break;
	case FunctionType::Kind::Require:
		print_assertion("assume", _args);
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
}

void ExpressionConverter::print_ext_method(
	FunctionType const& _type,
	Expression const& _call,
	vector<ASTPointer<Expression const>> const& _args
)
{
	if (auto call = NodeSniffer<MemberAccess>(_call).find())
	{
		print_method(_type, &call->expression(), _args);
	}
	else
	{
		throw runtime_error("Unable to extract address from external call.");
	}
}

void ExpressionConverter::print_method(
	FunctionType const& _type,
	Expression const* _ctx,
	vector<ASTPointer<Expression const>> const& _args
)
{
	auto &decl = dynamic_cast<FunctionDefinition const&>(_type.declaration());
	const bool is_mutable = (decl.stateMutability() != StateMutability::Pure);

	(*m_ostream) << m_converter.translate(decl).name << "(";

	if (is_mutable)
	{
		if (_ctx)
		{
			(*m_ostream) << "&";
			print_subexpression(*_ctx);
		}
		else
		{
			(*m_ostream) << "self";
		}
		(*m_ostream) << ", state";
	}

	for (unsigned int i = 0; i < _args.size(); ++i)
	{
		if (is_mutable || i > 0)
		{
			(*m_ostream) << ", ";
		}
		_args[i]->accept(*this);
	}

	(*m_ostream) << ")";
}

void ExpressionConverter::print_contract_ctor(
	Expression const& _call, vector<ASTPointer<Expression const>> const& _args
)
{
	if (auto contract_type = NodeSniffer<UserDefinedTypeName>(_call).find())
	{
		auto translation = m_converter.translate(*contract_type);
		(*m_ostream) << "Init_" << translation.name << "(";

		auto decl = contract_type->annotation().referencedDeclaration;
		if (auto contract = dynamic_cast<ContractDefinition const*>(decl))
		{
			if (contract->constructor())
			{
				(*m_ostream) << "nullptr, state";
				for (unsigned int i = 0; i < _args.size(); ++i)
				{
					(*m_ostream) << ", ";
					_args[i]->accept(*this);
				}
			}
		}
		else
		{
			throw runtime_error("Unable to resolve contract from TypeName.");
		}
		(*m_ostream) << ")";
	}
	else
	{
		throw runtime_error("Contract constructor called without TypeName.");
	}
}

void ExpressionConverter::print_payment(
	Expression const& _call, vector<ASTPointer<Expression const>> const& _args
)
{
	if (_args.size() != 1)
	{
		throw runtime_error("Payment calls require payment amount.");
	}
	else if (auto call = NodeSniffer<MemberAccess>(_call).find())
	{
		(*m_ostream) << "_pay(state, ";
		call->expression().accept(*this);
		(*m_ostream) << ", ";
		(_args[0])->accept(*this);
		(*m_ostream) << ")";	
	}
	else
	{
		throw runtime_error("Unable to extract address from payment call.");
	}
}

void ExpressionConverter::print_assertion(
	string _type, vector<ASTPointer<Expression const>> const& _args
)
{
	if (_args.empty())
	{
		throw runtime_error("Assertion requires condition.");
	}

	(*m_ostream) << _type << "(";
	(_args[0])->accept(*this);
	(*m_ostream) << ")";
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print_address_member(
	Expression const& _node, string const& _member
)
{
	if (_member == "balance")
	{
		// TODO(scottwe): add `_balance(state, addr)` call to runtime.
		(void) _node;
		throw runtime_error("Address balance not yet supported.");
	}
	else
	{
		throw runtime_error("Unrecognized Address member: " + _member);
	}
}

void ExpressionConverter::print_array_member(
	Expression const& _node, string const& _member
)
{
	if (_member == "length")
	{
		// TODO(scottwe): Decide on which "array features" should be allowed.
		(void) _node;
		throw runtime_error("Array-like lengths not yet supported.");
	}
	else
	{
		throw runtime_error("Unrecognized Array-like member: " + _member);
	}
}

void ExpressionConverter::print_adt_member(
	Expression const& _node, string const& _member
)
{
	print_subexpression(_node);
	(*m_ostream) << ".d_" << _member;
}

void ExpressionConverter::print_magic_member(
	TypePointer _type, string const& _member
)
{
	if (auto magic_type = dynamic_cast<MagicType const*>(_type))
	{
		auto res = m_magic_members.find({magic_type->kind(), _member});
		if (res == m_magic_members.end() || res->second == "")
		{
			throw runtime_error("Unable to resolve member of Magic type.");
		}
		(*m_ostream) << res->second;	
	}
	else
	{
		throw runtime_error("Resolution of MagicType failed in MemberAccess.");
	}
}

// -------------------------------------------------------------------------- //

}
}
}
