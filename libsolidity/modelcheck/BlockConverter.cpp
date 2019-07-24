/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#include <libsolidity/modelcheck/BlockConverter.h>

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

BlockConverter::BlockConverter(
	FunctionDefinition const& _func, TypeConverter const& _types
): m_body(_func.body()), m_types(_types)
{
	// TODO(scottwe): support multiple return types.
	if (_func.returnParameters().size() > 1)
	{
		throw runtime_error("Multiple return values not yet supported.");
	}
	else if (!_func.returnParameters().empty())
	{
		auto const& retvar = _func.returnParameters()[0];
		if (retvar->name() != "") m_rv = retvar;
	}

	m_decls.enter();
	for (auto const& arg : _func.parameters())
	{
		m_decls.record_declaration(*arg);
	}
}

// -------------------------------------------------------------------------- //

shared_ptr<CBlock> BlockConverter::convert()
{
	m_substmt = nullptr;
	m_last_block = nullptr;
	m_body.accept(*this);
	return m_last_block;
}

// -------------------------------------------------------------------------- //

bool BlockConverter::visit(Block const& _node)
{
	ScopedSwap<bool> top_level_swap(m_is_top_level, false);
	m_decls.enter();

	CBlockList stmts;
	shared_ptr<CVarDecl> rv;
	if (top_level_swap.old() && m_rv)
	{
		m_decls.record_declaration(*m_rv);
		rv = make_shared<CVarDecl>(m_types.translate(*m_rv).type, m_rv->name());
		stmts.push_back(rv);
	}
	for (auto const& stmt : _node.statements())
	{
		stmt->accept(*this);
		stmts.push_back(m_substmt);
	}
	if (top_level_swap.old() && m_rv)
	{
		stmts.push_back(make_shared<CReturn>(rv->id()));
	}
	m_last_block = make_shared<CBlock>(move(stmts));
	m_substmt = m_last_block;

	m_decls.exit();

	return false;
}

bool BlockConverter::visit(IfStatement const& _node)
{
	ExpressionConverter cond(_node.condition(), m_types, m_decls);
	CStmtPtr opt_substmt = nullptr;
	if (_node.falseStatement())
	{
		_node.falseStatement()->accept(*this);
		opt_substmt = m_substmt;
	}
	_node.trueStatement().accept(*this);
	m_substmt = make_shared<CIf>(cond.convert(), m_substmt, opt_substmt);

	return false;
}

bool BlockConverter::visit(WhileStatement const& _node)
{
	// TODO(scottwe): Ensure number of interations has finite bound.
	bool is_do_while = _node.isDoWhile();
	ExpressionConverter cond(_node.condition(), m_types, m_decls);
	_node.body().accept(*this);
	m_substmt = make_shared<CWhileLoop>(m_substmt, cond.convert(), is_do_while);

	return false;
}

bool BlockConverter::visit(ForStatement const& _node)
{
	m_decls.enter();

	// TODO(scottwe): Ensure number of interations has finite bound.
	CStmtPtr init = nullptr;
	if (_node.initializationExpression())
	{
		_node.initializationExpression()->accept(*this);
		init = m_substmt;
	}
	CExprPtr cond = nullptr;
	if (_node.condition())
	{
		ExpressionConverter condexpr(*_node.condition(), m_types, m_decls);
		cond = condexpr.convert();
	}
	CStmtPtr loop = nullptr;
	if (_node.loopExpression())
	{
		_node.loopExpression()->accept(*this);
		loop = m_substmt;
	}
	_node.body().accept(*this);
	m_substmt = make_shared<CForLoop>(init, cond, loop, m_substmt);

	m_decls.exit();

	return false;
}

bool BlockConverter::visit(InlineAssembly const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Inline assembly statement not yet supported.");
}

bool BlockConverter::visit(Return const& _node)
{
	CExprPtr retval_expr = nullptr;
	if (_node.expression())
	{
		ExpressionConverter retval(*_node.expression(), m_types, m_decls);
		retval_expr = retval.convert();
	}
	m_substmt = make_shared<CReturn>(retval_expr);
	return false;
}

bool BlockConverter::visit(Throw const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Throw statement not yet supported.");
}

bool BlockConverter::visit(EmitStatement const&)
{
	// TODO(scottwe): warn unchecked; emit statements may be used to audit.
	return false;
}

bool BlockConverter::visit(VariableDeclarationStatement const& _node)
{
	if (_node.declarations().size() > 1)
	{
		// TODO(scottwe): support multiple return values.
		throw runtime_error("Multiple return values are unsupported.");
	}
	else if (!_node.declarations().empty())
	{
		const auto &decl = *_node.declarations()[0];
		auto type = m_types.translate(decl).type;
		bool is_ref = decl.referenceLocation() == VariableDeclaration::Storage;
		m_decls.record_declaration(decl);

		CExprPtr val = nullptr;
		if (_node.initialValue())
		{
			auto const& expr = *_node.initialValue(); 
			ExpressionConverter val_converter(expr, m_types, m_decls, is_ref);
			val = val_converter.convert();
		}
		m_substmt = make_shared<CVarDecl>(type, decl.name(), is_ref, val);
	}

	return false;
}

bool BlockConverter::visit(ExpressionStatement const& _node)
{
	ExpressionConverter expr(_node.expression(), m_types, m_decls);
	m_substmt = make_shared<CExprStmt>(expr.convert());
	return false;
}

// -------------------------------------------------------------------------- //

void BlockConverter::endVisit(Continue const&)
{
	m_substmt = make_shared<CContinue>();
}

void BlockConverter::endVisit(Break const&)
{
	m_substmt = make_shared<CBreak>();
}

void BlockConverter::endVisit(PlaceholderStatement const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Placeholder statement not yet supported.");
}

// -------------------------------------------------------------------------- //

}
}
}
