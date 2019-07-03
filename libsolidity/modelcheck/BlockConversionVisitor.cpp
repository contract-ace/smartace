/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#include <libsolidity/modelcheck/BlockConversionVisitor.h>
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

BlockConversionVisitor::BlockConversionVisitor(
	FunctionDefinition const& _func,
	TypeTranslator const& _scope
): m_body(&_func.body()), m_scope(_scope)
{
	if (!_func.returnParameters().empty())
	{
		auto const& retvar = _func.returnParameters()[0];
		if (retvar->name() != "")
		{
			m_retvar = retvar;
		}
	}
}

// -------------------------------------------------------------------------- //

void BlockConversionVisitor::print(ostream& _stream)
{
	m_ostream = &_stream;
	m_is_top_level = true;
	m_is_loop_statement = false;
	m_body->accept(*this);
	m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool BlockConversionVisitor::visit(Block const& _node)
{
	bool print_returns = (m_is_top_level && m_retvar);
	m_is_top_level = false;

	m_decls.enter();
	(*m_ostream) << "{" << endl;

	if (print_returns)
	{
		vector<ASTPointer<VariableDeclaration>> decls({m_retvar});
		VariableDeclarationStatement decl_stmt(
			langutil::SourceLocation(), nullptr, decls, nullptr);
		decl_stmt.accept(*this);
		(*m_ostream) << endl;
	}

	for (auto const& stmt : _node.statements())
	{
		stmt->accept(*this);
		(*m_ostream) << endl;
	}

	if (print_returns)
	{
		auto id = make_shared<Identifier>(
			langutil::SourceLocation(), make_shared<string>(m_retvar->name()));
		Return ret_stmt(langutil::SourceLocation(), nullptr, id);
		ret_stmt.accept(*this);
		(*m_ostream) << endl;
	}

	(*m_ostream) << "}";
	m_decls.exit();

	return false;
}

bool BlockConversionVisitor::visit(PlaceholderStatement const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Placeholder statement not yet supported.");
}

bool BlockConversionVisitor::visit(IfStatement const& _node)
{
	ExpressionConversionVisitor condition_visitor(
		_node.condition(), m_scope, m_decls
	);

	(*m_ostream) << "if (";
	condition_visitor.print(*m_ostream);
	(*m_ostream) << ")" << endl;
	_node.trueStatement().accept(*this);
	if (_node.falseStatement())
	{
		(*m_ostream) << endl << "else" << endl;
		_node.falseStatement()->accept(*this);
	}

	return false;
}

bool BlockConversionVisitor::visit(WhileStatement const& _node)
{
	// TODO(scottwe): Ensure number of interations has finite bound.
	ExpressionConversionVisitor condition_visitor(
		_node.condition(), m_scope, m_decls
	);

	if (_node.isDoWhile())
	{
		(*m_ostream) << "do" << endl;
		_node.body().accept(*this);
		(*m_ostream) << endl << "while (";
		condition_visitor.print(*m_ostream);
		(*m_ostream) << ")";
		end_statement();
	}
	else
	{
		(*m_ostream) << "while (";
		condition_visitor.print(*m_ostream);
		(*m_ostream) << ")" << endl;
		_node.body().accept(*this);
	}

	return false;
}

bool BlockConversionVisitor::visit(ForStatement const& _node)
{
	m_decls.enter();

	(*m_ostream) << "for (";
	print_loop_statement(_node.initializationExpression());
	(*m_ostream) << "; ";

	// TODO(scottwe): Ensure number of interations has finite bound.
	if (_node.condition())
	{
		ExpressionConversionVisitor condition_visitor(
			*_node.condition(), m_scope, m_decls
		);
		condition_visitor.print(*m_ostream);
	}

	(*m_ostream) << "; ";
	print_loop_statement(_node.loopExpression());
	(*m_ostream) << ")" << endl;
	_node.body().accept(*this);

	m_decls.exit();

	return false;
}

bool BlockConversionVisitor::visit(Continue const&)
{
	(*m_ostream) << "continue";
	end_statement();
	return false;
}

bool BlockConversionVisitor::visit(InlineAssembly const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Inline assembly statement not yet supported.");
}

bool BlockConversionVisitor::visit(Break const&)
{
	(*m_ostream) << "break";
	end_statement();
	return false;
}

bool BlockConversionVisitor::visit(Return const& _node)
{
	(*m_ostream) << "return";
	if (_node.expression())
	{
		ExpressionConversionVisitor retval_visitor(
			*_node.expression(), m_scope, m_decls
		);
		(*m_ostream) << " ";
		retval_visitor.print(*m_ostream);
	}
	end_statement();
	return false;
}

bool BlockConversionVisitor::visit(Throw const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Throw statement not yet supported.");
}

bool BlockConversionVisitor::visit(EmitStatement const&)
{
	return false;
}

bool BlockConversionVisitor::visit(VariableDeclarationStatement const& _node)
{
	if (_node.declarations().size() > 1)
	{
		throw runtime_error("Multiple return values are unsupported.");
	}
	else if (!_node.declarations().empty())
	{
		const auto &decl = *_node.declarations()[0];
		m_decls.record_declaration(decl);

		(*m_ostream) << m_scope.translate(decl.type()).type
					 << " "
					 << decl.name();

		if (_node.initialValue())
		{
			ExpressionConversionVisitor value_visitor(
				*_node.initialValue(), m_scope, m_decls
			);
			(*m_ostream) << " = ";
			value_visitor.print(*m_ostream);
		}

		end_statement();
	}

	return false;
}

bool BlockConversionVisitor::visit(ExpressionStatement const& _node)
{
	ExpressionConversionVisitor expr_visitor(
		_node.expression(), m_scope, m_decls
	);
	expr_visitor.print(*m_ostream);
	end_statement();
	return false;
}

// -------------------------------------------------------------------------- //

void BlockConversionVisitor::print_loop_statement(Statement const* _node)
{
	if (_node)
	{
		m_is_loop_statement = true;
		_node->accept(*this);
		m_is_loop_statement = false;
	}
}

void BlockConversionVisitor::end_statement()
{
	if (!m_is_loop_statement)
	{
		(*m_ostream) << ";";
	}
}

// -------------------------------------------------------------------------- //

}
}
}
