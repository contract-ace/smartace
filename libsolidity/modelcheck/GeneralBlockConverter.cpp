/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#include <libsolidity/modelcheck/BlockConverter.h>

#include <libsolidity/modelcheck/Contract.h>
#include <libsolidity/modelcheck/ExpressionConverter.h>
#include <libsolidity/modelcheck/Function.h>
#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/TranslationLiterals.h>
#include <libsolidity/modelcheck/TypeClassification.h>
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

GeneralBlockConverter::GeneralBlockConverter(
	std::vector<ASTPointer<VariableDeclaration>> const& _args,
	Block const& _body,
	TypeConverter const& _types,
	bool _manage_pay,
	bool _is_payable
): M_BODY(_body)
 , M_TYPES(_types)
 , M_MANAGE_PAY(_manage_pay)
 , M_IS_PAYABLE(_is_payable)
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
	return ExpressionConverter(_expr, M_TYPES, m_decls, _ref).convert();
}

// -------------------------------------------------------------------------- //

CStmtPtr & GeneralBlockConverter::last_substmt()
{
	return m_substmt;
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
		auto const state = make_shared<CIdentifier>("state", true);
		auto V = state->access("value")->access("v");
		if (M_MANAGE_PAY && M_IS_PAYABLE)
		{
			string const BAL_MEMBER = ContractUtilities::balance_member();
			auto const self = make_shared<CIdentifier>("self", true);
			auto BAL = self->access(BAL_MEMBER)->access("v");
			stmts.push_back(CBinaryOp(BAL, "+=", V).stmt());
		}
		else if (M_MANAGE_PAY)
		{
			stmts.push_back(make_shared<CFuncCall>("sol_require", CArgList{
				make_shared<CBinaryOp>(V, "==", Literals::ZERO), Literals::ZERO
			})->stmt());
		}
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

bool GeneralBlockConverter::visit(EmitStatement const&)
{
	// TODO(scottwe): warn unchecked; emit statements may be used to audit.
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

		m_decls.record_declaration(DECL);

		CExprPtr val = nullptr;
		if (_node.initialValue())
		{
			val = expand(*_node.initialValue(), IS_REF);
			val = FunctionUtilities::try_to_wrap(*DECL.type(), move(val));
		}

		auto const TYPE = M_TYPES.get_type(DECL);
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

}
}
}
