/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <libsolidity/modelcheck/VariableScopeResolver.h>
#include <functional>
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
class BlockToModelVisitor : public ASTConstVisitor
{
public:
    // Creates a C model code generator for a given block of Solidity code.
    BlockToModelVisitor(
        Block const& _body,
        TypeTranslator const& _scope
    );

    // Generates a human-readable block of C code, from the given block.
    void print(std::ostream& _stream);

protected:
	bool visit(Block const& _node) override;
    bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(Continue const&) override;
	bool visit(Break const&) override;
	bool visit(Return const& _node) override;
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;
	bool visit(UnaryOperation const& _node) override;
	bool visit(BinaryOperation const& _node) override;
	bool visit(Identifier const& _node) override;
    bool visit(Literal const& _node) override;

private:
    Block const* m_body;
    TypeTranslator const& m_scope;
	VariableScopeResolver m_decls;

	std::ostream* m_ostream = nullptr;

	std::hash<std::string> m_hasher;

	bool m_is_loop_statement = false;

	static long long int literal_to_number(Literal const& _node);

	void print_subexpression(Expression const& _node);
	void print_loop_statement(ASTNode const* _node);
	void end_statement();
};

}
}
}
