#include <libsolidity/modelcheck/model/Block.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/Expression.h>
#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/Types.h>

#include <algorithm>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

GeneralBlockConverter::GeneralBlockConverter(
	vector<ASTPointer<VariableDeclaration>> const& _args,
	vector<ASTPointer<VariableDeclaration>> const& _rvs,
	Block const& _body,
	shared_ptr<AnalysisStack const> _stack,
	bool _manage_pay,
	bool _is_payable
): m_stack(_stack)
 , M_BODY(_body)
 , M_MANAGE_PAY(_manage_pay)
 , M_IS_PAYABLE(_is_payable)
 , M_BLOCKTYPE(determine_block_type(_rvs, _stack))
{
	m_decls.enter();
	for (auto const& arg : _args)
	{
		m_decls.record_declaration(*arg);
	}
}

GeneralBlockConverter::~GeneralBlockConverter() = default;

// -------------------------------------------------------------------------- //

shared_ptr<CBlock> GeneralBlockConverter::convert()
{
	m_top_block = nullptr;
	M_BODY.accept(*this);
	return m_top_block;
}

// -------------------------------------------------------------------------- //

CExprPtr GeneralBlockConverter::expand(Expression const& _expr, bool _ref)
{
	bool const INITS = block_type() == BlockType::Initializer;
	return ExpressionConverter(_expr, m_stack, m_decls, _ref, INITS).convert();
}

// -------------------------------------------------------------------------- //

CStmtPtr GeneralBlockConverter::last_substmt()
{
	return m_substmt;
}

// -------------------------------------------------------------------------- //

GeneralBlockConverter::BlockType GeneralBlockConverter::block_type() const
{
	return M_BLOCKTYPE;
}

// -------------------------------------------------------------------------- //

bool GeneralBlockConverter::has_retval() const
{
	return (block_type() == BlockType::Operation);
}

// -------------------------------------------------------------------------- //

bool GeneralBlockConverter::visit(Block const& _node)
{
	CBlockList stmts;

	// Drops the top level flag (for nested blocks) and enters a new scope.
	ScopedSwap<bool> top_level_swap(m_is_top_level, false);
	m_decls.enter();

	// Performs setup specific to the top-level block.
	if (top_level_swap.old())
	{
		if (M_MANAGE_PAY && M_IS_PAYABLE) add_value_handler(stmts);
		enter(stmts, m_decls);
	}

	// Converts each block statement.
	for (auto const& stmt : _node.statements())
	{
		stmt->accept(*this);
		stmts.push_back(last_substmt());
	}

	// Top level blocks must set-up and tear-down the context, i.e. return.
	if (top_level_swap.old())
	{
		exit(stmts, m_decls);
		m_top_block = make_shared<CBlock>(move(stmts));
	}
	else
	{
		new_substmt<CBlock>(move(stmts));
	}

	// Exits the scope and backtracks the AST.
	m_decls.exit();
	return false;
}

bool GeneralBlockConverter::visit(IfStatement const& _node)
{
	// Converts the false branch, if applicable.
	CStmtPtr opt_substmt = nullptr;
	if (_node.falseStatement())
	{
		_node.falseStatement()->accept(*this);
		opt_substmt = last_substmt();
	}

	// Converts the true branch.
	_node.trueStatement().accept(*this);

	// Assembles both branches into an if.
	new_substmt<CIf>(expand(_node.condition()), last_substmt(), opt_substmt);
	return false;
}

bool GeneralBlockConverter::visit(WhileStatement const& _node)
{
	_node.body().accept(*this);
	new_substmt<CWhileLoop>(
		last_substmt(), expand(_node.condition()), _node.isDoWhile()
	);

	return false;
}

bool GeneralBlockConverter::visit(ForStatement const& _node)
{
	// The loop condition is in its own scope.
	m_decls.enter();

	// All loop statements are optional. They are converted if applicable.
	CStmtPtr init = nullptr;
	if (_node.initializationExpression())
	{
		_node.initializationExpression()->accept(*this);
		init = last_substmt();
	}
	CExprPtr cond = nullptr;
	if (_node.condition())
	{
		cond = expand(*_node.condition());
	}
	CStmtPtr loop = nullptr;
	if (_node.loopExpression())
	{
		_node.loopExpression()->accept(*this);
		loop = last_substmt();
	}

	// Assembles the for statement.
	_node.body().accept(*this);
	new_substmt<CForLoop>(init, cond, loop, last_substmt());

	// Exits the scope and backtracks.
	m_decls.exit();
	return false;
}

bool GeneralBlockConverter::visit(InlineAssembly const&)
{
	throw runtime_error("Inline assembly is not supported.");
}

bool GeneralBlockConverter::visit(Throw const&)
{
	throw runtime_error("Throw is deprecated.");
}

bool GeneralBlockConverter::visit(EmitStatement const& _node)
{
	auto const& call = _node.eventCall();
	CBlockList stmts;

	// Expands emit parameters.
	for (auto arg : call.arguments())
	{
		expand_expr_into_stmt(*arg);
		stmts.push_back(last_substmt());
	}

	// Expands emit.
	string event = get_ast_string(&call);
	CFuncCallBuilder sol_emit_call("sol_emit");
	sol_emit_call.push(make_shared<CStringLiteral>(event));
	stmts.push_back(make_shared<CExprStmt>(sol_emit_call.merge_and_pop()));
	new_substmt<CExprStmt>(sol_emit_call.merge_and_pop());

	// Generates emit block.
	new_substmt<CBlock>(move(stmts));
	return false;
}

bool GeneralBlockConverter::visit(VariableDeclarationStatement const& _node)
{
	if (_node.declarations().size() > 1)
	{
		// TODO(scottwe): support multiple return values.
		throw runtime_error("Multiple return values are unsupported.");
	}
	else if (!_node.declarations().empty())
	{
		auto const &DECL = (*_node.declarations()[0]);
		bool const IS_REF = decl_is_ref(DECL);

		CExprPtr val = nullptr;
		if (_node.initialValue())
		{
			val = expand(*_node.initialValue(), IS_REF);
			val = InitFunction::wrap(*DECL.type(), move(val));
		}

		if (DECL.annotation().type->category() == Type::Category::Contract)
		{
			// These are implicitly references.
			throw runtime_error("Contract variable declarations unsupported.");
		}

		m_decls.record_declaration(DECL);

		auto const TYPE = m_stack->types()->get_type(DECL);
		auto const NAME = m_decls.resolve_declaration(DECL);
		new_substmt<CVarDecl>(TYPE, NAME, IS_REF, val);
	}

	return false;
}

bool GeneralBlockConverter::visit(ExpressionStatement const& _node)
{
	ExpressionCleaner c_cleaner(_node.expression());
	expand_expr_into_stmt(c_cleaner.clean());
	return false;
}

// -------------------------------------------------------------------------- //

void GeneralBlockConverter::endVisit(Continue const&)
{
	new_substmt<CContinue>();
}

void GeneralBlockConverter::endVisit(Break const&)
{
	new_substmt<CBreak>();
}

// -------------------------------------------------------------------------- //

GeneralBlockConverter::BlockType GeneralBlockConverter::determine_block_type(
	vector<ASTPointer<VariableDeclaration>> const& _rvs,
	shared_ptr<AnalysisStack const> _stack
)
{
	if (_rvs.empty())
	{
		return BlockType::Action;
	}
	else if (_stack->allocations()->retval_is_allocated(*_rvs[0]))
	{
		// TODO(scottwe): this doesn't generalize to tuple return values.
		if (_rvs.size() > 1)
		{
			throw runtime_error("BlockType::Initializer assumes 1 rv.");
		}
		return BlockType::Initializer;
	}
	else
	{
		return BlockType::Operation;
	}
}

// -------------------------------------------------------------------------- //

void GeneralBlockConverter::add_value_handler(CBlockList & _block)
{
	auto const VALUE_SYM = CallStateUtilities::Field::Value;
	auto const VALUE_FLD = CallStateUtilities::get_name(VALUE_SYM);
	auto const VALUE = make_shared<CIdentifier>(VALUE_FLD, false)->access("v");

	auto const PAID_SYM = CallStateUtilities::Field::Paid;
	auto const PAID_FLD = CallStateUtilities::get_name(PAID_SYM);
	auto const PAID = make_shared<CIdentifier>(PAID_FLD, false)->access("v");

	auto const SELF = make_shared<CIdentifier>("self", true);
	auto BAL = SELF->access(ContractUtilities::balance_member())->access("v");

	auto CHECKS = make_shared<CBinaryOp>(PAID, "==", Literals::ONE);
	auto CHANGE = CBinaryOp(BAL, "+=", VALUE).stmt();
	_block.push_back(make_shared<CIf>(CHECKS, CHANGE));
}

// -------------------------------------------------------------------------- //

void GeneralBlockConverter::expand_expr_into_stmt(Expression const& _expr)
{
	// This could be an assignment, which is special-cased on tuples.
	if (auto op = dynamic_cast<Assignment const*>(&_expr))
	{
		// Determines if this is a tuple assignment.
		ExpressionCleaner l_cleaner(op->leftHandSide());
		if (auto lhs = dynamic_cast<TupleExpression const*>(&l_cleaner.clean()))
		{
			ExpressionCleaner r_cleaner(op->rightHandSide());
			expand_tuple_assign_into_stmt(*lhs, r_cleaner.clean());
			return;
		}
	}

	// Default case (not tuple assignment).
	new_substmt<CExprStmt>(expand(_expr));
}

void GeneralBlockConverter::expand_tuple_assign_into_stmt(
	TupleExpression const& _lhs, Expression const& _rhs
)
{
	CBlockList stmts;
	vector<shared_ptr<CVarDecl>> tmp_vars;

	// Checks and processes RHS.
	if (auto func_rhs = dynamic_cast<FunctionCall const*>(&_rhs))
	{
		FunctionCallAnalyzer call(*func_rhs);

		// Generates temporary variables for each return value.
		for (auto entry : call.method_decl().returnParameters())
		{
			string name = "blockvar_" + to_string(tmp_vars.size());
			auto type = m_stack->types()->get_type(*entry.get());
			auto decl = make_shared<CVarDecl>(type, name, false);
			tmp_vars.push_back(decl);
			stmts.push_back(decl);
		}

		// Generates the call.
		vector<CExprPtr> rv_ids;
		ExpressionConverter expr(*func_rhs, m_stack, m_decls);
		for (size_t i = 1; i < tmp_vars.size(); ++i)
		{
			auto var = tmp_vars[i];
			rv_ids.push_back(make_shared<CReference>(var->id()));
		}
		expr.set_aux_rvs(rv_ids);

		// Generates the call statement.
		auto dst = tmp_vars[0]->id()->access("v");
		auto src = expr.convert();
		auto assign = make_shared<CBinaryOp>(dst, "=", src);
		stmts.push_back(assign->stmt());
	}
	else if (auto tuple_rhs = dynamic_cast<TupleExpression const*>(&_rhs))
	{
		// Generates temporary variables for each variable on the RHS.
		for (auto entry : _lhs.components())
		{
			string name = "blockvar_" + to_string(tmp_vars.size());
			auto type = m_stack->types()->get_type(*entry.get());
			auto decl = make_shared<CVarDecl>(type, name, false);
			tmp_vars.push_back(decl);
			stmts.push_back(decl);
		}

		// Assigns RHS values to temp vars.
		for (size_t i = 0; i < tuple_rhs->components().size(); ++i)
		{
			auto rhs_id = expand(*tuple_rhs->components()[i].get());
			auto tmp_id = tmp_vars[i]->id()->access("v");
			auto assign = make_shared<CBinaryOp>(tmp_id, "=", rhs_id);
			stmts.push_back(assign->stmt());
		}
	}
	else
	{
		throw runtime_error("Unexpected LHS tuple.");
	}

	// Assigns temp vars to LHS.
	for (size_t i = 0; i < _lhs.components().size(); ++i)
	{
		if (auto expr = _lhs.components()[i])
		{
			auto lhs_id = expand(*expr);
			auto tmp_id = tmp_vars[i]->id()->access("v");
			auto assign = make_shared<CBinaryOp>(lhs_id, "=", tmp_id);
			stmts.push_back(assign->stmt());
		}
	}

	// Generates tuple-like statement.
	new_substmt<CBlock>(move(stmts));
}

// -------------------------------------------------------------------------- //

}
}
}
