#include <libsolidity/modelcheck/model/Expression.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/utils/CallState.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/Types.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ExpressionConverter::ExpressionConverter(
	Expression const& _expr,
	std::shared_ptr<AnalysisStack const> _stack,
	VariableScopeResolver const& _decls,
	bool _is_ref,
	bool _is_init
): M_EXPR(&_expr)
 , M_DECLS(_decls)
 , m_stack(_stack)
 , m_is_init(_is_init)
 , m_find_ref(_is_ref)
{
}

// -------------------------------------------------------------------------- //

CExprPtr ExpressionConverter::convert()
{
	m_subexpr = nullptr;
	m_last_assignment = nullptr;
	M_EXPR->accept(*this);
	return m_subexpr;
}

// -------------------------------------------------------------------------- //

bool ExpressionConverter::visit(Conditional const& _node)
{
	_node.condition().accept(*this);
	auto subexpr_1 = m_subexpr;

	_node.trueExpression().accept(*this);
	auto subexpr_2 = m_subexpr;

	_node.falseExpression().accept(*this);

	m_subexpr = make_shared<CCond>(
		move(subexpr_1), move(subexpr_2), move(m_subexpr)
	);

	return false;
}

bool ExpressionConverter::visit(Assignment const& _node)
{
	// Finds base identifier and detects contract instantiation.
	auto const ID = LValueSniffer<Identifier>(_node.leftHandSide()).find();
	if (ID && ID->annotation().type->category() == Type::Category::Contract)
	{
		ScopedSwap<Identifier const*> swap(m_last_assignment, ID);
		_node.rightHandSide().accept(*this);
		return false;
	}

	// Establishes RHS
	{
		auto const FINDING_REF = (ID && m_stack->types()->is_pointer(*ID));
		ScopedSwap<bool> swap(m_find_ref, FINDING_REF);
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

	// Equals LHS to RHS.
	{
		ScopedSwap<bool> swap(m_lval, true);
		if (auto map = LValueSniffer<IndexAccess>(_node.leftHandSide()).find())
		{
			// TODO: "Write" should not be hard-coded.
			FlatIndex idx(*map);
			auto record = m_stack->types()->map_db().resolve(idx.decl());
			generate_mapping_call("Write", move(record), move(idx), move(rhs));
		}
		else
		{
			_node.leftHandSide().accept(*this);
			m_subexpr = make_shared<CBinaryOp>(m_subexpr, "=", move(rhs));
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
	_node.subExpression().accept(*this);

	bool const IS_PREFIX = _node.isPrefixOperation();
	if (_node.getOperator() == Token::Delete)
	{
		// TODO(scottwe)
		throw runtime_error("Delete not yet supported.");
	}
	else
	{
		string const OP = TokenTraits::friendlyName(_node.getOperator());
		m_subexpr = make_shared<CUnaryOp>(OP, move(m_subexpr), IS_PREFIX);
	}

	return false;
}

bool ExpressionConverter::visit(BinaryOperation const& _node)
{
	auto const& LHS = _node.leftExpression();
	auto const& RHS = _node.rightExpression();
	generate_binary_op(LHS, _node.getOperator(), RHS);
	return false;
}

bool ExpressionConverter::visit(FunctionCall const& _node)
{
	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND == FunctionCallKind::FunctionCall)
	{
		print_function(_node);
	}
	else if (KIND == FunctionCallKind::TypeConversion)
	{
		print_cast(_node);
	}
	else if (KIND == FunctionCallKind::StructConstructorCall)
	{
		print_struct_ctor(_node);
	}
	else
	{
		throw runtime_error("FunctionCall encountered of unknown kind.");
	}
	return false;
}

bool ExpressionConverter::visit(MemberAccess const& _node)
{
	auto const EXPR_TYPE = _node.expression().annotation().type;
	ScopedSwap<bool> find_ref(m_find_ref, false);

	bool auto_unwrapped = false;
	switch (EXPR_TYPE->category())
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
		print_magic_member(EXPR_TYPE, _node.memberName());
		break;
	case Type::Category::TypeType:
		print_enum_member(EXPR_TYPE, _node.memberName());
		auto_unwrapped = true;
		break;
	default:
		throw runtime_error("MemberAccess applied to invalid type.");
	}

	if (find_ref.old())
	{
		m_subexpr = make_shared<CReference>(move(m_subexpr));
	}
	else if (is_wrapped_type(*_node.annotation().type) && !auto_unwrapped)
	{
		m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), "v");
	}

	return false;
}

bool ExpressionConverter::visit(IndexAccess const& _node)
{
	switch (_node.baseExpression().annotation().type->category())
	{
	case Type::Category::Mapping:
		{
			FlatIndex idx(_node);
			auto record = m_stack->types()->map_db().resolve(idx.decl());

			if (idx.indices().size() != record.key_types.size())
			{
				throw runtime_error("Partial map lookup unsupported.");
			}
			else if (m_find_ref)
			{
				throw runtime_error("Map references unsupported.");
			}

			// TODO: "Read" should not be hard-coded.
			generate_mapping_call("Read", record, idx, nullptr);

			if (is_wrapped_type(*_node.annotation().type))
			{
				m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), "v");
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
	bool const IS_REF = m_stack->types()->is_pointer(_node);

	m_subexpr = make_shared<CIdentifier>(
		M_DECLS.resolve_identifier(_node), IS_REF
	);

	// TODO: this code is duplicated
	if (m_find_ref && !IS_REF)
	{
		m_subexpr = make_shared<CReference>(move(m_subexpr));
	}
	else if (is_wrapped_type(*_node.annotation().type))
	{
		m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), "v");
	}

	return false;
}

bool ExpressionConverter::visit(Literal const& _node)
{
	switch (_node.token())
	{
	case Token::TrueLiteral:
		m_subexpr = Literals::ONE;
		break;
	case Token::FalseLiteral:
		m_subexpr = Literals::ZERO;
		break;
	case Token::Number:
		if (m_is_address_cast)
		{
			auto lit = _node.annotation().type->literalValue(&_node);
			auto const& lit_name = AbstractAddressDomain::literal_name(lit);
			m_subexpr = make_shared<CIdentifier>(lit_name, false);
		}
		else
		{
			m_subexpr = make_shared<CIntLiteral>(literal_to_number(_node));
		}
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
	auto subexpr_1 = m_subexpr;
	_rhs.accept(*this);

	string const OP = TokenTraits::friendlyName(_op);
	if (_op == Token::SAR || _op == Token::SHR || _op == Token::Exp)
	{
		throw runtime_error("Unsupported binary operator:" + OP);
	}

	m_subexpr = make_shared<CBinaryOp>(move(subexpr_1), OP, move(m_subexpr));
}

void ExpressionConverter::generate_mapping_call(
	string const& _op,
	MapDeflate::FlatMap const& _map,
	FlatIndex const& _idx,
	CExprPtr _v
)
{
	// The type of baseExpression is an array, so it is not a wrapped type.
	CFuncCallBuilder builder(_op + "_" + _map.name);
	builder.push(_idx.base(), m_stack, M_DECLS, true);

	{
		auto const& IDX_END = _idx.indices().end();
		auto const& KEY_END = _map.key_types.end();

		auto idx_itr = _idx.indices().begin();
		auto key_itr = _map.key_types.begin();

		while (idx_itr != IDX_END && key_itr != KEY_END)
		{
			auto const& IDX = (**idx_itr);
			auto const* type = (*key_itr)->annotation().type;
			builder.push(IDX, m_stack, M_DECLS, false, type);
			++idx_itr;
			++key_itr;
		}
	}

	if (_v) builder.push(move(_v), _map.value_type->annotation().type);
	m_subexpr = builder.merge_and_pop();
}

CExprPtr ExpressionConverter::get_initializer_context() const
{
	if (m_last_assignment)
	{
		return make_shared<CReference>(make_shared<CIdentifier>(
			M_DECLS.resolve_identifier(*m_last_assignment), false
		));
	}
	else
	{
		return make_shared<CIdentifier>(InitFunction::INIT_VAR, true);
	}
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::push_arglist(
	SolArgList const& _args,
	SolDeclList const& _decls,
	CFuncCallBuilder & _builder,
	size_t _offset
) const
{
	for (size_t i = 0; i < _args.size(); ++i)
	{
		push_arg(*_args[i], *_decls[_offset + i], _builder);
	}
}

void ExpressionConverter::push_arg(
	Expression const& _arg,
	VariableDeclaration const& _decl,
	CFuncCallBuilder & _builder
) const
{
	auto const TYPE = _decl.type();
	bool is_ref = (_decl.referenceLocation() == VariableDeclaration::Storage);
	_builder.push(_arg, m_stack, M_DECLS, is_ref, TYPE);
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print_struct_ctor(FunctionCall const& _call)
{
	if (auto id = NodeSniffer<Identifier>(_call.expression()).find())
	{
		auto const& def = dynamic_cast<StructDefinition const&>(
			*id->annotation().referencedDeclaration
		);

		auto builder = InitFunction(*m_stack->types(), def).call_builder();
		push_arglist(_call.arguments(), def.members(), builder);
		m_subexpr = builder.merge_and_pop();
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

	auto const& BASE_EXPR = *_call.arguments()[0];
	auto base_type = BASE_EXPR.annotation().type;
	auto cast_type = _call.annotation().type;

	if (auto BASE_RAT = dynamic_cast<RationalNumberType const*>(base_type))
	{
		base_type = BASE_RAT->integerType();
	}
	if (auto CAST_RAT = dynamic_cast<RationalNumberType const*>(cast_type))
	{
		cast_type = CAST_RAT->integerType();
	}

	if (!base_type || cast_type->category() == Type::Category::FixedPoint ||
		!cast_type || base_type->category() == Type::Category::FixedPoint)
	{
		throw runtime_error("FixedPoint conversion is unsupported in solc.");
	}

	{
		bool const IS_ADDR = (cast_type->category() == Type::Category::Address);
		ScopedSwap<bool> scope(m_is_address_cast, IS_ADDR);
		BASE_EXPR.accept(*this);
	}

	if (base_type->category() == Type::Category::Address)
	{
		if (auto cast_int = dynamic_cast<IntegerType const*>(cast_type))
		{
			if (cast_int->isSigned())
			{
				BASE_EXPR.accept(*this);
			}
			else
			{
				m_subexpr = make_shared<CCast>(move(m_subexpr), "unsigned int");
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
					m_subexpr = make_shared<CCast>(move(m_subexpr), "int");
				}
				else
				{
					m_subexpr = make_shared<CCast>(move(m_subexpr), "unsigned int");
				}
			}
		}
		else if (cast_type->category() == Type::Category::Address)
		{
			if (!base_int->isSigned())
			{
				m_subexpr = make_shared<CCast>(move(m_subexpr), "int");
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
			string const FIELD = ContractUtilities::address_member();
			m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), FIELD);
			m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), "v");
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

void ExpressionConverter::print_function(FunctionCall const& _call)
{
	FunctionCallAnalyzer calldata(_call);

	// TODO: error logging.
	FunctionCallAnalyzer::CallGroup group = calldata.classify();
	if (group == FunctionCallAnalyzer::CallGroup::Method)
	{
		print_method(calldata);
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Delegate)
	{
		throw runtime_error("Delegate calls are unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Constructor)
	{
		print_contract_ctor(_call);
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Send)
	{
		print_payment(_call, true);
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Transfer)
	{
		print_payment(_call, false);
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Crypto)
	{
		throw runtime_error("Cryptographic calls are unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Destruct)
	{
		throw runtime_error("Self-destruction is unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Revert)
	{
		print_revert(_call.arguments());
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Assert)
	{
		print_assertion("sol_assert", _call.arguments());
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Require)
	{
		print_assertion("sol_require", _call.arguments());
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Logging)
	{
		throw runtime_error("Logging calls are unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Blockhash)
	{
		throw runtime_error("block.blockhash(<val>)` is unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::AddMod)
	{
		// TODO(scottwe): overflow free `assert(z > 0); return (x + y) % z;`.
		throw runtime_error("AddMod not yet supported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::MulMod)
	{
		// TODO(scottwe): overflow free `assert(z > 0); return (x * y) % z;`.
		throw runtime_error("MulMod not yet supported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Push)
	{
		throw runtime_error("`<array>.push(<val>)` is unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::Pop)
	{
		throw runtime_error("`<array>.pop(<val>)` not yet supported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::NewArray)
	{
		throw runtime_error("`new <array>` not yet supported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::ABI)
	{
		throw runtime_error("ABI calls are unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::GasLeft)
	{
		throw runtime_error("gasLeft() is unsupported.");
	}
	else if (group == FunctionCallAnalyzer::CallGroup::UnhandledCall)
	{
		throw runtime_error("Unexpected function call type.");
	}
}

void ExpressionConverter::print_method(FunctionCallAnalyzer const& _calldata)
{
	FunctionSpecialization call(_calldata.decl());

	// Determines call name and locality of call.
	bool is_ext_call = (!_calldata.is_super() && _calldata.context());
	if (_calldata.is_super())
	{
		call = FunctionSpecialization(call.func(), M_DECLS.spec()->use_by());
	}
	else if (!(is_ext_call || call.source().isLibrary()))
	{
        auto const& user = M_DECLS.spec()->use_by();
        string const& target = call.func().name();

		auto match = find_named_match<FunctionDefinition>(&user, target);
		call = FunctionSpecialization(*match, user);
	}

	// Determines if builder's method produces return value by pointer.
	bool rv_is_wrapped = false;
	bool rv_is_ptr = false;
	if (!_calldata.type().returnParameterTypes().empty())
	{
		auto const& rv = (*_calldata.type().returnParameterTypes()[0]);
		rv_is_wrapped = is_wrapped_type(rv);
		rv_is_ptr = (rv.category() == Type::Category::Contract);
	}
	CFuncCallBuilder builder(call.name(0), rv_is_ptr);	

	// Sets state for the next call.
	size_t param_idx = 0;
	if (call.source().isLibrary())
	{
		if (_calldata.context())
		{
			auto const DECL = call.func().parameters()[param_idx];
			push_arg(*_calldata.context(), *DECL, builder);
			++param_idx;
		}
	}
	else
	{
		if (is_ext_call)
		{
			_calldata.context()->accept(*this);

			// Checks if the context is taken by reference (not param or rv).
			if (!dynamic_cast<FunctionCall const*>(_calldata.context()))
			{
				auto ref = expr_to_decl(*_calldata.context());
				if (ref && ref->referenceLocation() != VariableDeclaration::CallData)
				{
					m_subexpr = make_shared<CReference>(move(m_subexpr));
				}
			}

			builder.push(move(m_subexpr));
		}
		else
		{
			builder.push(make_shared<CIdentifier>("self", true));
		}
		pass_next_call_state(_calldata, builder, is_ext_call);
	}

	// Passes dest if contract construction is in use.
	if (m_is_init) builder.push(get_initializer_context());

	// Pushes all user provided arguments and generates the function call.
	push_arglist(_calldata.args(), call.func().parameters(), builder, param_idx);
	m_subexpr = builder.merge_and_pop();

	// Unwraps the return value, if it is a wrapped type.
	if (rv_is_wrapped)
	{
		m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), "v");
	}
}

void ExpressionConverter::print_contract_ctor(FunctionCall const& _call)
{
	if (auto def = NodeSniffer<UserDefinedTypeName>(_call).find())
	{
		auto builder = InitFunction(*m_stack->types(), *def).call_builder();
		auto const DECL = def->annotation().referencedDeclaration;
		if (auto contract = dynamic_cast<ContractDefinition const*>(DECL))
		{	
			builder.push(get_initializer_context());
			pass_next_call_state(FunctionCallAnalyzer(_call), builder, true);

			if (auto const& ctor = contract->constructor())
			{
				push_arglist(_call.arguments(), ctor->parameters(), builder);
			}
		}
		else
		{
			throw runtime_error("Unable to resolve contract from TypeName.");
		}

		m_subexpr = builder.merge_and_pop();
	}
	else
	{
		throw runtime_error("Contract constructor called without TypeName.");
	}
}

void ExpressionConverter::print_payment(FunctionCall const& _call, bool _nothrow)
{
	const AddressType ADR_TYPE(StateMutability::Payable);
	const IntegerType AMT_TYPE(256, IntegerType::Modifier::Unsigned);

	auto const& args = _call.arguments();
	if (args.size() != 1)
	{
		throw runtime_error("Payment calls require payment amount.");
	}
	else if (auto call = NodeSniffer<MemberAccess>(_call).find())
	{
		// Computes source balance.
		auto const BAL_MEMBER = ContractUtilities::balance_member();
		auto src = make_shared<CIdentifier>("self", true);
		auto srcbal = make_shared<CMemberAccess>(src, BAL_MEMBER);

		// Generates source balance and amount arguments.
		auto srcbalref = make_shared<CReference>(srcbal);
		auto const& AMT = *args[0];

		// Searches for a base contract.
		auto const& dst = call->expression();
		FunctionCall const* cast = NodeSniffer<FunctionCall>(dst).find();
		if (cast)
		{
			auto const BASE = cast->expression().annotation().type->category();
			if (BASE == Type::Category::Contract) 
			{
				if (cast->annotation().kind != FunctionCallKind::TypeConversion)
				{
					cast = nullptr;
				}
			}
			else
			{
				cast = nullptr;
			}
		}

		// Determines if recipient is known.
		CExprPtr recipient;
		TypePointer recipient_type;
		if (cast)
		{
			throw runtime_error("Send to contract not yet supported.");
		}
		else
		{
			dst.accept(*this);
			recipient = m_subexpr;
			recipient_type = (&ADR_TYPE);
			// TODO: warn about approximation.
			// TODO: map target to address space.
		}

		// Generates the call.
		CFuncCallBuilder fn(_nothrow ? CallState::SEND : CallState::TRANSFER);
		fn.push(srcbalref);
		fn.push(recipient, recipient_type);
		fn.push(AMT, m_stack, M_DECLS, false, &AMT_TYPE);
		m_subexpr = fn.merge_and_pop();

		// TODO: handle fallbacks.
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

	// TODO(scottwe): support for messages.
	CFuncCallBuilder builder(_type);
	const InaccessibleDynamicType RAW_TYPE;
	builder.push(*_args[0], m_stack, M_DECLS, false, &RAW_TYPE);
	builder.push(Literals::ZERO);
	m_subexpr = builder.merge_and_pop();
}

void ExpressionConverter::print_revert(SolArgList const&)
{
	// TODO(scottwe): support for messages.
	CFuncCallBuilder builder("sol_require");
	builder.push(Literals::ZERO);
	builder.push(Literals::ZERO);
	m_subexpr = builder.merge_and_pop();
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::pass_next_call_state(
	FunctionCallAnalyzer const& _call, CFuncCallBuilder & _builder, bool _is_ext
)
{
	CExprPtr v;
	if (_call.value())
	{
		v = ExpressionConverter(
			*_call.value(), m_stack, M_DECLS, false, m_is_init
		).convert();
		v = InitFunction::wrap(*ContractUtilities::balance_type(), move(v));
	}
	m_stack->environment()->compute_next_state_for(_builder, _is_ext, move(v));
}

// -------------------------------------------------------------------------- //

void ExpressionConverter::print_address_member(
	Expression const& _node, string const& _member
)
{
	if (_member == "balance")
	{
		auto const* fcall = NodeSniffer<FunctionCall>(_node, false).find();
		if (!fcall)
		{
			throw runtime_error("Balance of arbitrary address not supported.");
		}
		if (fcall->annotation().kind != FunctionCallKind::TypeConversion)
		{
			throw runtime_error("Balance expects cast to address.");
		}

		auto base = fcall->arguments()[0];
		if (base->annotation().type->category() != Type::Category::Contract)
		{
			throw runtime_error("Balance expects contract cast to address.");
		}
		
		base->accept(*this);
		string const FIELD = ContractUtilities::balance_member();
		m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), FIELD);
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

void ExpressionConverter::print_adt_member(Expression const& _node, string _member)
{
	_node.accept(*this);
	auto name = M_DECLS.rewrite(move(_member), false, VarContext::STRUCT);
	m_subexpr = make_shared<CMemberAccess>(move(m_subexpr), move(name));
}

void ExpressionConverter::print_magic_member(TypePointer _t, string _member)
{
	auto const TYPE = CallStateUtilities::parse_magic_type(*_t, move(_member));
	auto name = CallStateUtilities::get_name(TYPE);
	m_subexpr = make_shared<CIdentifier>(move(name), false);
}

void ExpressionConverter::print_enum_member(
	TypePointer _t, string const& _member
)
{
	const auto* WRAPPED_T = dynamic_cast<TypeType const*>(_t);
	const auto* ENUM_T = dynamic_cast<EnumType const*>(WRAPPED_T->actualType());
	if (!ENUM_T)
	{
		throw runtime_error("EnumValue lacks EnumType.");
	}
	m_subexpr = make_shared<CIntLiteral>(ENUM_T->memberValue(_member));
}

// -------------------------------------------------------------------------- //

}
}
}
