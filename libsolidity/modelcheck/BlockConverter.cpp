/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#include <libsolidity/modelcheck/BlockConverter.h>

#include <libsolidity/modelcheck/ExpressionConverter.h>
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
	FunctionDefinition const& _func,
	TypeConverter const& _converter
): m_body(&_func.body()), m_converter(_converter)
{
	// TODO(scottwe): support multiple return types.
	if (_func.returnParameters().size() > 1)
	{
		throw runtime_error("Multiple return values not yet supported.");
	}
	else if (!_func.returnParameters().empty())
	{
		auto const& retvar = _func.returnParameters()[0];
		if (retvar->name() != "")
		{
			m_retvar = retvar;
		}
	}

	m_decls.enter();
	for (auto const& arg : _func.parameters())
	{
		m_decls.record_declaration(*arg);
	}
}

// -------------------------------------------------------------------------- //

void BlockConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
	m_body->accept(*this);
}

// -------------------------------------------------------------------------- //

bool BlockConverter::visit(Block const& _node)
{
	ScopedSwap<bool> top_level_swap(m_is_top_level, false);

	m_decls.enter();
	(*m_ostream) << "{";

	if (top_level_swap.old() && m_retvar)
	{
		vector<ASTPointer<VariableDeclaration>> decls({m_retvar});
		VariableDeclarationStatement decl_stmt(
			langutil::SourceLocation(), nullptr, decls, nullptr
		);
		decl_stmt.accept(*this);
	}

	for (auto const& stmt : _node.statements())
	{
		stmt->accept(*this);
	}

	if (top_level_swap.old() && m_retvar)
	{
		auto id = make_shared<Identifier>(
			langutil::SourceLocation(), make_shared<string>(m_retvar->name())
		);
		Return ret_stmt(langutil::SourceLocation(), nullptr, id);
		ret_stmt.accept(*this);
	}

	(*m_ostream) << "}";
	m_decls.exit();

	return false;
}

bool BlockConverter::visit(PlaceholderStatement const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Placeholder statement not yet supported.");
}

bool BlockConverter::visit(IfStatement const& _node)
{
	ScopedSwap<bool> if_statement_swap(m_is_if_statement_body, true);

	(*m_ostream) << (if_statement_swap.old() ? " " : "") << "if(";
	ExpressionConverter(
		_node.condition(), m_converter, m_decls
	).print(*m_ostream);
	(*m_ostream) << ")";
	_node.trueStatement().accept(*this);
	if (_node.falseStatement())
	{
		(*m_ostream) << "else";
		_node.falseStatement()->accept(*this);
	}

	return false;
}

bool BlockConverter::visit(WhileStatement const& _node)
{
	// TODO(scottwe): Ensure number of interations has finite bound.
	ExpressionConverter condition_visitor(
		_node.condition(), m_converter, m_decls
	);

	if (_node.isDoWhile())
	{
		(*m_ostream) << "do";
		_node.body().accept(*this);
		(*m_ostream) << "while(";
		condition_visitor.print(*m_ostream);
		(*m_ostream) << ")";
		end_statement();
	}
	else
	{
		(*m_ostream) << "while(";
		condition_visitor.print(*m_ostream);
		(*m_ostream) << ")";
		_node.body().accept(*this);
	}

	return false;
}

bool BlockConverter::visit(ForStatement const& _node)
{
	m_decls.enter();

	(*m_ostream) << "for(";
	print_loop_statement(_node.initializationExpression());
	(*m_ostream) << ";";

	// TODO(scottwe): Ensure number of interations has finite bound.
	if (_node.condition())
	{
		ExpressionConverter(
			*_node.condition(), m_converter, m_decls
		).print(*m_ostream);
	}

	(*m_ostream) << ";";
	print_loop_statement(_node.loopExpression());
	(*m_ostream) << ")";
	_node.body().accept(*this);

	m_decls.exit();

	return false;
}

bool BlockConverter::visit(Continue const&)
{
	(*m_ostream) << "continue";
	end_statement();
	return false;
}

bool BlockConverter::visit(InlineAssembly const& _node)
{
	(void) _node;
	// TODO(scottwe): implement.
	throw runtime_error("Inline assembly statement not yet supported.");
}

bool BlockConverter::visit(Break const&)
{
	(*m_ostream) << "break";
	end_statement();
	return false;
}

bool BlockConverter::visit(Return const& _node)
{
	(*m_ostream) << "return";
	if (_node.expression())
	{
		(*m_ostream) << " ";
		ExpressionConverter(
			*_node.expression(), m_converter, m_decls
		).print(*m_ostream);
	}
	end_statement();
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
		bool is_ref = decl.referenceLocation() == VariableDeclaration::Storage;
		m_decls.record_declaration(decl);

		(*m_ostream) << m_converter.translate(decl).type
		             << (is_ref ? "*" : " ") << decl.name();

		if (_node.initialValue())
		{
			(*m_ostream) << "=";
			ExpressionConverter(
				*_node.initialValue(), m_converter, m_decls, is_ref
			).print(*m_ostream);
		}

		end_statement();
	}

	return false;
}

bool BlockConverter::visit(ExpressionStatement const& _node)
{
	ExpressionConverter(
		_node.expression(), m_converter, m_decls
	).print(*m_ostream);
	end_statement();
	return false;
}

// -------------------------------------------------------------------------- //

void BlockConverter::print_loop_statement(Statement const* _node)
{
	if (_node)
	{
		ScopedSwap<bool> loop_statement_swap(m_is_loop_statement, true);
		_node->accept(*this);
	}
}

void BlockConverter::end_statement()
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
