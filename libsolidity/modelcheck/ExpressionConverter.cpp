/**
 * @date 2019
 * Utility visitor to convert Solidity expressions into verifiable code.
 */

#include <libsolidity/modelcheck/ExpressionConverter.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/Utility.h>
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
	TypeConverter const& _types,
	VariableScopeResolver const& _decls,
	bool _is_ref
): m_expr(&_expr), m_types(_types), m_decls(_decls), m_find_ref(_is_ref)
{
}

// -------------------------------------------------------------------------- //

CExprPtr ExpressionConverter::convert()
{
	m_subexpr = nullptr;
	m_expr->accept(*this);
	return m_subexpr;
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
	_node.condition().accept(*this);
	auto subexpr1 = m_subexpr;
	_node.trueExpression().accept(*this);
	auto subexpr2 = m_subexpr;
	_node.falseExpression().accept(*this);
	m_subexpr = make_shared<CCond>(subexpr1, subexpr2, m_subexpr);
	return false;
}

bool ExpressionConverter::visit(Assignment const& _node)
{
	{
		auto id = LValueSniffer<Identifier>(_node.leftHandSide()).find();
		ScopedSwap<bool> swap(m_find_ref, id && m_types.is_pointer(*id));
		if (_node.assignmentOperator() != Token::Assign)
		{
			generate_binary_op(
				_node.leftHandSide(),
				TokenTraits::AssignmentToBinaryOp(_node.assignmentOperator()),
				_node.rightHandSide()
			);
		}
		else
		{
			_node.rightHandSide().accept(*this);
		}
	}
	auto rhs = m_subexpr;

	{
		ScopedSwap<bool> swap(m_lval, true);
		if (auto map = LValueSniffer<IndexAccess>(_node.leftHandSide()).find())
		{
			string name = m_types.translate(*map).name;
			generate_mapping_call("Write", name, *map, rhs);
		}
		else
		{
			_node.leftHandSide().accept(*this);
			m_subexpr = make_shared<CBinaryOp>(m_subexpr, "=", rhs);
		}
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
	string op;
	switch (_node.getOperator())
	{
	case Token::Not:
	case Token::BitNot:
		op = "!";
		break;
	case Token::Delete:
		// TODO(scottwe)
		throw runtime_error("Delete not yet supported.");
	case Token::Inc:
	case Token::Dec:
		op = TokenTraits::friendlyName(_node.getOperator());
		break;
	default:
		throw runtime_error("UnaryOp not yet supported.");
	}
	
	_node.subExpression().accept(*this);
	m_subexpr = make_shared<CUnaryOp>(op, m_subexpr, _node.isPrefixOperation());

	return false;
}

bool ExpressionConverter::visit(BinaryOperation const& _node)
{
	auto const& lhs = _node.leftExpression();
	auto const& rhs = _node.rightExpression();
	generate_binary_op(lhs, _node.getOperator(), rhs);
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
		print_cast(_node);
		break;
	case FunctionCallKind::StructConstructorCall:
		print_struct_ctor(_node.expression(), _node.arguments());
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
			string map_name = m_types.translate(_node).name;
			if (m_find_ref)
			{
				generate_mapping_call("Ref", map_name, _node, nullptr);
			}
			else if (m_lval)
			{
				generate_mapping_call("Ref", map_name, _node, nullptr);
				m_subexpr = make_shared<CDereference>(m_subexpr);
			}
			else
			{
				generate_mapping_call("Read", map_name, _node, nullptr);
			}
		}
		break;
	default:
		throw runtime_error("IndexAccess applied to unsupported type.");
	}
	return false;
}

bool ExpressionConverter::visit(Identifier const& _node)
{
	m_subexpr = make_shared<CIdentifier>(
		m_decls.resolve_identifier(_node), m_types.is_pointer(_node)
	);
	if (m_find_ref)
	{
		m_subexpr = make_shared<CReference>(m_subexpr);
	}
	return false;
}

bool ExpressionConverter::visit(Literal const& _node)
{
	switch (_node.token())
	{
	case Token::TrueLiteral:
		m_subexpr = make_shared<CIntLiteral>(1);
		break;
	case Token::FalseLiteral:
		m_subexpr = make_shared<CIntLiteral>(0);
		break;
	case Token::Number:
		m_subexpr = make_shared<CIntLiteral>(literal_to_number(_node));
		break;
	case Token::StringLiteral:
		m_subexpr = make_shared<CIntLiteral>(hash<string>()(_node.value()));
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
		return (num * 1000000000000);
	case Literal::SubDenomination::Finney:
		return (num * 1000000000000000);
	case Literal::SubDenomination::Ether:
		return (num * 1000000000000000000);
	case Literal::SubDenomination::Minute:
		return (num * 60);
	case Literal::SubDenomination::Hour:
		return (num * 60 * 60);
	case Literal::SubDenomination::Day:
		return (num * 60 * 60 * 24);
	case Literal::SubDenomination::Week:
		return (num * 60 * 60 * 24 * 7);
	case Literal::SubDenomination::Year:
		return (num * 60 * 60 * 24 * 365);
	default:
		return num;
	}
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::generate_binary_op(
	Expression const& _lhs, Token _op, Expression const& _rhs
)
{
	_lhs.accept(*this);
	auto subexpr1 = m_subexpr;
	_rhs.accept(*this);

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
		m_subexpr = make_shared<CBinaryOp>(
			subexpr1, TokenTraits::friendlyName(_op), m_subexpr
		);
		break;
	default:
		throw runtime_error("BinOp not yet supported.");
	}	
}

void ExpressionConverter::generate_mapping_call(
	string const& _op, string const& _id, IndexAccess const& _map, CExprPtr _v
)
{
	CArgList cargs;
	{
		ScopedSwap<bool> swap(m_find_ref, true);
		_map.baseExpression().accept(*this);
		cargs.push_back(m_subexpr);
	}
	{
		ScopedSwap<bool> swap(m_find_ref, false);
		_map.indexExpression()->accept(*this);
		cargs.push_back(m_subexpr);
	}
	if (_v) cargs.push_back(move(_v));
	m_subexpr = make_shared<CFuncCall>(_op + "_" + _id, move(cargs));
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print_struct_ctor(
	Expression const& _struct, SolArgList const& _args
)
{
	if (auto struct_id = NodeSniffer<Identifier>(_struct).find())
	{
		CArgList cargs;
		for (auto arg : _args)
		{
			arg->accept(*this);
			cargs.push_back(m_subexpr);
		}

		auto name = m_types.translate(*struct_id).name;
		m_subexpr = make_shared<CFuncCall>("Init_" + name, move(cargs));
	}
	else
	{
		throw runtime_error("Struct constructor called without identifier.");
	}
}

void ExpressionConverter::print_cast(FunctionCall const& _call)
{
	if (_call.arguments().size() != 1)
	{
		throw runtime_error("Unable to typecast multiple values in one call.");
	}

	auto const& base_expr = *_call.arguments()[0];
	auto base_type = base_expr.annotation().type;
	auto cast_type = _call.annotation().type;

	if (auto base_rat = dynamic_cast<RationalNumberType const*>(base_type))
	{
		base_type = base_rat->integerType();
	}
	if (auto cast_rat = dynamic_cast<RationalNumberType const*>(cast_type))
	{
		cast_type = cast_rat->integerType();
	}

	if (!base_type || cast_type->category() == Type::Category::FixedPoint ||
		!cast_type || base_type->category() == Type::Category::FixedPoint)
	{
		throw runtime_error("FixedPoint conversion is unsupported in solc.");
	}

	base_expr.accept(*this);
	if (base_type->category() == Type::Category::Address)
	{
		if (auto cast_int = dynamic_cast<IntegerType const*>(cast_type))
		{
			if (cast_int->isSigned())
			{
				base_expr.accept(*this);
			}
			else
			{
				m_subexpr = make_shared<CCast>(m_subexpr, "unsigned int");
			}
		}
		else if (cast_type->category() == Type::Category::Enum)
		{
			// TODO(scottwe): implement.
			throw runtime_error("Enums are not yet supported.");
		}
		else if (cast_type->category() != Type::Category::Address)
		{
			throw runtime_error("Unsupported address cast.");
		}
	}
	else if (auto base_int = dynamic_cast<IntegerType const*>(base_type))
	{
		if (auto cast_int = dynamic_cast<IntegerType const*>(cast_type))
		{
			// TODO(scottwe): take into account bitwidth.
			// TODO(scottwe): are sign semantics the same in Solidity?
			if (base_int->isSigned() != cast_int->isSigned())
			{
				if (cast_int->isSigned())
				{
					m_subexpr = make_shared<CCast>(m_subexpr, "int");
				}
				else
				{
					m_subexpr = make_shared<CCast>(m_subexpr, "unsigned int");
				}
			}
		}
		else if (cast_type->category() == Type::Category::Address)
		{
			if (!base_int->isSigned())
			{
				m_subexpr = make_shared<CCast>(m_subexpr, "int");
			}
		}
		else if (cast_type->category() == Type::Category::Enum)
		{
			// TODO(scottwe): implement.
			throw runtime_error("Enums are not yet supported.");
		}
		else
		{
			throw runtime_error("Unsupported integer cast.");
		}
		
	}
	else if (base_type->category() == Type::Category::StringLiteral)
	{
		// TODO(scottwe): implement.
		throw runtime_error("String conversion is unsupported.");
	}
	else if (base_type->category() == Type::Category::FixedBytes)
	{
		// TODO(scottwe): implement.
		throw runtime_error("Byte arrays are not yet supported.");
	}
	else if (base_type->category() == Type::Category::Bool)
	{
		if (cast_type->category() != Type::Category::Bool)
		{
			throw runtime_error("Unsupported bool cast.");
		}
	}
	else if (base_type->category() == Type::Category::Array)
	{
		// TODO(scottwe): implement.
		throw runtime_error("Arrays are not yet supported.");
	}
	else if (base_type->category() == Type::Category::Contract)
	{
		if (cast_type->category() == Type::Category::Contract)
		{
			// TODO(scottwe): which casts should be allowed?
			throw runtime_error("Contract/Contract casts unimplemented.");
		}
		else if (cast_type->category() == Type::Category::Address)
		{
			// TODO(scottwe): implement Address/Contract association.
			throw runtime_error("Contract/Address casts unimplemented");
		}
		else
		{
			throw runtime_error("Unsupported Contract cast.");
		}
	}
	else if (base_type->category() == Type::Category::Enum)
	{
		// TODO(scottwe): implement.
		throw runtime_error("Enums are not yet supported.");
	}
	else
	{
		throw runtime_error("Conversion applied to unexpected type.");
	}
}

void ExpressionConverter::print_function(
	Expression const& _call, SolArgList const& _args
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
	FunctionType const& _type, Expression const& _call, SolArgList const& _args
)
{
	auto call = NodeSniffer<MemberAccess>(_call).find();
	if (!call)
	{
		throw runtime_error("Unable to extract address from external call.");
	}
	print_method(_type, &call->expression(), _args);
}

void ExpressionConverter::print_method(
	FunctionType const& _type, Expression const* _ctx, SolArgList const& _args
)
{
	auto &decl = dynamic_cast<FunctionDefinition const&>(_type.declaration());
	const bool is_mutable = (decl.stateMutability() != StateMutability::Pure);

	CArgList carg;
	if (is_mutable)
	{
		if (_ctx)
		{
			_ctx->accept(*this);
			auto id = LValueSniffer<Identifier>(*_ctx).find();
			if (!(id && m_types.is_pointer(*id)))
			{
				m_subexpr = make_shared<CReference>(m_subexpr);
			}
			carg.push_back(m_subexpr);
		}
		else
		{
			carg.push_back(make_shared<CIdentifier>("self", true));
		}
		carg.push_back(make_shared<CIdentifier>("state", true));
	}
	for (auto arg : _args)
	{
		arg->accept(*this);
		carg.push_back(m_subexpr);
	}

	auto name = m_types.translate(decl).name;
	m_subexpr = make_shared<CFuncCall>(name, move(carg));
}

void ExpressionConverter::print_contract_ctor(
	Expression const& _call, SolArgList const& _args
)
{
	if (auto contract_type = NodeSniffer<UserDefinedTypeName>(_call).find())
	{
		CArgList cargs;
		auto decl = contract_type->annotation().referencedDeclaration;
		if (auto contract = dynamic_cast<ContractDefinition const*>(decl))
		{
			if (contract->constructor())
			{
				cargs.push_back(make_shared<CIdentifier>("nullptr", true));
				cargs.push_back(make_shared<CIdentifier>("state", true));
				for (auto arg : _args)
				{
					arg->accept(*this);
					cargs.push_back(m_subexpr);
				}
			}
		}
		else
		{
			throw runtime_error("Unable to resolve contract from TypeName.");
		}

		auto name = m_types.translate(*contract_type).name;
		m_subexpr = make_shared<CFuncCall>("Init_" + name, move(cargs));
	}
	else
	{
		throw runtime_error("Contract constructor called without TypeName.");
	}
}

void ExpressionConverter::print_payment(
	Expression const& _call, SolArgList const& _args
)
{
	if (_args.size() != 1)
	{
		throw runtime_error("Payment calls require payment amount.");
	}
	else if (auto call = NodeSniffer<MemberAccess>(_call).find())
	{
		CArgList cargs{make_shared<CIdentifier>("state", true)};
		call->expression().accept(*this);
		cargs.push_back(m_subexpr);
		(_args[0])->accept(*this);
		cargs.push_back(m_subexpr);
		m_subexpr = make_shared<CFuncCall>("_pay", move(cargs));
	}
	else
	{
		throw runtime_error("Unable to extract address from payment call.");
	}
}

void ExpressionConverter::print_assertion(string _type, SolArgList const& _args)
{
	if (_args.empty())
	{
		throw runtime_error("Assertion requires condition.");
	}

	(_args[0])->accept(*this);
	m_subexpr = make_shared<CFuncCall>(_type, CArgList{m_subexpr});
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
	ScopedSwap<bool> swap(m_find_ref, false);
	_node.accept(*this);

	m_subexpr = make_shared<CMemberAccess>(m_subexpr, "d_" + _member);
	if (swap.old())
	{
		m_subexpr = make_shared<CReference>(m_subexpr);
	}
}

void ExpressionConverter::print_magic_member(
	TypePointer _type, string const& _member
)
{
	auto magic_type = dynamic_cast<MagicType const*>(_type);
	if (!magic_type)
	{
		throw runtime_error("Resolution of MagicType failed in MemberAccess.");
	}
	auto res = m_magic_members.find({magic_type->kind(), _member});
	if (res == m_magic_members.end() || res->second == "")
	{
		throw runtime_error("Unable to resolve member of Magic type.");
	}
	m_subexpr = make_shared<CIdentifier>(res->second, false);
}

// -------------------------------------------------------------------------- //

}
}
}
