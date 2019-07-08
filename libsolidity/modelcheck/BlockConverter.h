/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <libsolidity/modelcheck/VariableScopeResolver.h>
#include <ostream>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * A utility visitor, designed to convert Solidity code blocks into executable
 * C code. This is meant to be used a utility when converting a full Solidity
 * source unit. This splits data structure conversion from instruction
 * conversion.
 */
class BlockConverter : public ASTConstVisitor
{
public:
    // Constructs a printer for the C code corresponding to a Solidity function.
	// The converter should provide translations for all typed ASTNodes.
    BlockConverter(
        FunctionDefinition const& _func,
		TypeConverter const& _converter
    );

    // Streams a human-readable block of C code, from the given Solidity
	// function.
    void print(std::ostream& _stream);

protected:
	bool visit(Block const& _node) override;
	bool visit(PlaceholderStatement const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(Continue const& _node) override;
	bool visit(InlineAssembly const& _node) override;
	bool visit(Break const& _node) override;
	bool visit(Return const& _node) override;
	bool visit(Throw const& _node) override;
	bool visit(EmitStatement const&) override;
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;

private:
    Block const* m_body;
	TypeConverter const& m_converter;
	VariableScopeResolver m_decls;

	ASTPointer<VariableDeclaration> m_retvar = nullptr;

	std::ostream* m_ostream = nullptr;

	bool m_is_loop_statement;
	bool m_is_top_level;

	void print_loop_statement(Statement const* _node);
	void end_statement();
};

}
}
}
