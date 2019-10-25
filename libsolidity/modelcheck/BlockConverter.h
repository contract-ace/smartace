/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <libsolidity/modelcheck/VariableScopeResolver.h>
#include <ostream>
#include <type_traits>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * A utility visitor, designed to convert Solidity code blocks into executable
 * C-code. This is meant to be used a utility when converting a full Solidity
 * source unit. This splits data structure conversion from instruction
 * conversion.
 * 
 * This implementation is generalized, and is meant to be overriden by
 * concrete cases (functions, modifiers, etc).
 */
class GeneralBlockConverter : public ASTConstVisitor
{
public:
	// Constructs a printer for the C code corresponding to a Solidity function.
	// The converter should provide translations for all typed ASTNodes.
	GeneralBlockConverter(
		std::vector<ASTPointer<VariableDeclaration>> const& _args,
		Block const& _body,
		TypeConverter const& _types
	);

	virtual ~GeneralBlockConverter() = 0;

	// Generates a SimpleCGenerator representation of the Solidity function.
	std::shared_ptr<CBlock> convert();

protected:
	// Allows top-level setup and teardown.
	virtual void enter(CBlockList & _stmts, VariableScopeResolver & _decls) = 0;
	virtual void exit(CBlockList & _stmts, VariableScopeResolver & _decls) = 0;

	// Utility to expand a condition into c-code.
	CExprPtr expand(Expression const& _expr, bool _ref = false);

	// Utilities to manage sub-statements.
	template <typename Stmt, typename... Args> 
	void new_substmt(Args&&... _args)
	{
		static_assert(
			std::is_constructible<Stmt, Args...>::value,
			"During block conversion, a CStmt was constructed from bad types."
		);
		static_assert(
			std::is_base_of<CStmt, Stmt>::value,
			"During block conversion, substmt was set to non CStmt."
		);
		m_substmt = std::make_shared<Stmt>(std::forward<Args>(_args)...);
	}
	CStmtPtr & last_substmt();

	bool visit(Block const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(InlineAssembly const&) override;
	bool visit(Throw const& _node) override;
	bool visit(EmitStatement const&) override;
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;

	void endVisit(Break const&) override;
	void endVisit(Continue const&) override;

private:
	Block const& M_BODY;
	TypeConverter const& M_TYPES;

	VariableScopeResolver m_decls;

	CStmtPtr m_substmt;
	std::shared_ptr<CBlock> m_top_block;

	bool m_is_top_level = true;
};

// -------------------------------------------------------------------------- //

/**
 * Specializes GeneralBlockConverter for FunctionDefinitions, adding support for
 * named and unnamed return values.
 */
class FunctionBlockConverter : public GeneralBlockConverter
{
public:
	//
	FunctionBlockConverter(
		FunctionDefinition const& _func, TypeConverter const& _types
	);

	~FunctionBlockConverter() override = default;

protected:
	void enter(CBlockList & _stmts, VariableScopeResolver & _decls) override;
	void exit(CBlockList & _stmts, VariableScopeResolver & _decls) override;

	bool visit(Return const& _node) override;

private:
	TypeConverter const& M_TYPES;

	ASTPointer<VariableDeclaration> m_rv = nullptr;
};

// -------------------------------------------------------------------------- //

/**
 * Specializes GeneralBlockConverter to handle ModifierDefinition semantics.
 */
class ModifierConverter : public GeneralBlockConverter
{
protected:
	bool visit(Return const& _node) override;

	void endVisit(PlaceholderStatement const&) override;
};

// -------------------------------------------------------------------------- //

}
}
}
