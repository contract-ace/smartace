#include <libsolidity/modelcheck/model/Block.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/Expression.h>
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
	TypeAnalyzer const& _converter,
	CallState const& _statedata,
	AllocationGraph const& _alloc_graph,
	bool _manage_pay,
	bool _is_payable
): M_CONVERTER(_converter)
 , M_BODY(_body)
 , M_STATEDATA(_statedata)
 , M_MANAGE_PAY(_manage_pay)
 , M_IS_PAYABLE(_is_payable)
 , M_BLOCKTYPE(determine_block_type(_rvs, _alloc_graph))
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
	bool const IS_INIT = block_type() == BlockType::Initializer;
	return ExpressionConverter(
		_expr, M_CONVERTER, M_STATEDATA, m_decls, _ref, IS_INIT
	).convert();
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
	switch (block_type())
	{
	case BlockType::Operation:
	case BlockType::AddressRef:
		return true;
	default:
		return false;
	}
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

	// If this is the top level, perform teardown, and set return value. If not,
	// propogate the results.
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
	// TODO(scottwe): Ensure number of interations has finite bound.

	_node.body().accept(*this);
	new_substmt<CWhileLoop>(
		last_substmt(), expand(_node.condition()), _node.isDoWhile()
	);

	return false;
}

bool GeneralBlockConverter::visit(ForStatement const& _node)
{
	// TODO(scottwe): Ensure number of iterations has finite bound.

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
	// TODO(scottwe): implement.
	throw runtime_error("Inline assembly statement not yet supported.");
}

bool GeneralBlockConverter::visit(Throw const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Throw statement not yet supported.");
}

bool GeneralBlockConverter::visit(EmitStatement const& _node)
{
	auto const& LOC = _node.eventCall().location();
	auto const& SRC = LOC.source->source();
	string event = SRC.substr(LOC.start, LOC.end - LOC.start);
	event.erase(remove(event.begin(), event.end(), '\n'), event.end());

	CFuncCallBuilder sol_emit_call("sol_emit");
	sol_emit_call.push(make_shared<CStringLiteral>(event));
	new_substmt<CExprStmt>(sol_emit_call.merge_and_pop());

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
		auto const &DECL = *_node.declarations()[0];
		bool const IS_REF
			= DECL.referenceLocation() == VariableDeclaration::Storage;
		if (DECL.annotation().type->category() == Type::Category::Contract)
		{
			// These are implicitly references.
			throw runtime_error("Contract variable declarations unsupported.");
		}

		m_decls.record_declaration(DECL);

		CExprPtr val = nullptr;
		if (_node.initialValue())
		{
			val = expand(*_node.initialValue(), IS_REF);
			val = InitFunction::wrap(*DECL.type(), move(val));
		}

		auto const TYPE = M_CONVERTER.get_type(DECL);
		new_substmt<CVarDecl>(
			TYPE, m_decls.resolve_declaration(DECL), IS_REF, val
		);
	}

	return false;
}

bool GeneralBlockConverter::visit(ExpressionStatement const& _node)
{
	new_substmt<CExprStmt>(expand(_node.expression()));
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
	AllocationGraph const& _alloc_graph
)
{
	if (_rvs.empty())
	{
		return BlockType::Action;
	}
	else if (_rvs.size() > 1)
	{
		throw runtime_error("Multiple return values not yet supported.");
	}
	else if (_alloc_graph.retval_is_allocated(*_rvs[0]))
	{
		return BlockType::Initializer;
	}
	else if (_rvs[0]->type()->category() == Type::Category::Contract)
	{
		return BlockType::AddressRef;
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

}
}
}
