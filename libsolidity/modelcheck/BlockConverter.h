/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/SimpleCCore.h>
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
        FunctionDefinition const& _func, TypeConverter const& _types
    );

	// Generates a SimpleCGenerator representation of the Solidity function.
    CStmtPtr convert();

protected:
	bool visit(Block const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(InlineAssembly const& _node) override;
	bool visit(Return const& _node) override;
	bool visit(Throw const& _node) override;
	bool visit(EmitStatement const&) override;
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;

	void endVisit(Break const&) override;
	void endVisit(Continue const&) override;
	void endVisit(PlaceholderStatement const& _node) override;

private:
    Block const& m_body;
	TypeConverter const& m_types;
	VariableScopeResolver m_decls;

	ASTPointer<VariableDeclaration> m_retvar = nullptr;
	CStmtPtr m_substmt;

	bool m_is_top_level = true;
};

}
}
}
