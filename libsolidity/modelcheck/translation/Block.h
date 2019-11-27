/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Details.h>
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
 * Static helper utilities for block-like codeflow generation.
 */
class BlockUtilities
{
public:
	// Generates the payment call.
	static void add_value_handler(CBlockList & _block);
};

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
	// The converter should provide translations for all typed ASTNodes. This
	// assumes that user function params will be set up, corresponding to _args.
	// The _body will be expanded, using these parameters, along with _type.
	GeneralBlockConverter(
		std::vector<ASTPointer<VariableDeclaration>> const& _args,
		Block const& _body,
		CallState const& _statedata,
		TypeConverter const& _types,
		bool _manage_pay,
		bool _is_payable
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
	CallState const& M_STATEDATA;
	TypeConverter const& M_TYPES;

	bool const M_MANAGE_PAY;
	bool const M_IS_PAYABLE;

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
	// Creates a block converter for _func's main body. It is assumed that
	// _types is able to resolve all types in the AST of the source unit(s)
	// associated with _func.
	FunctionBlockConverter(
		FunctionDefinition const& _func,
		CallState const& _statedata,
		TypeConverter const& _types
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
 * A single block will correspond to a single modifier, but specialized to the
 * given function. Otherwise, the size of the code could explode exponentially.
 * 
 * For instance, if func _f has n modifiers, each with at least m > 1
 * placeholder operations, then inline(_f) has at least m^n blocks to expand.
 */
class ModifierBlockConverter : public GeneralBlockConverter
{
public:
	// Creates a block converter for the (_i)-th modifier of function _func. It
	// is assumed that _types is able to resolve all types in the AST of the
	// source unit(s) associated with _func.
	ModifierBlockConverter(
		FunctionDefinition const& _func,
		size_t _i,
		CallState const& _statedata,
		TypeConverter const& _types
	);

	~ModifierBlockConverter() override = default;

protected:
	void enter(CBlockList & _stmts, VariableScopeResolver & _decls) override;
	void exit(CBlockList & _stmts, VariableScopeResolver &) override;

	bool visit(Return const&) override;

	void endVisit(PlaceholderStatement const&) override;

private:
	CallState const& M_STATEDATA;
	TypeConverter const& M_TYPES;
	std::vector<ASTPointer<VariableDeclaration>> const& M_TRUE_PARAMS;
	std::vector<ASTPointer<VariableDeclaration>> const& M_USER_PARAMS;
	std::vector<ASTPointer<Expression>> const* M_USER_ARGS;
	std::string const M_NEXT_CALL;

	VariableScopeResolver m_shadow_decls;
	std::shared_ptr<CVarDecl> m_rv = nullptr;

	// Internal constructor parameters.
	struct Context
	{
		Context(FunctionDefinition const& _func, size_t _i);
		FunctionDefinition const& func;
		ModifierInvocation const* curr = nullptr;
		ASTNode const* next = nullptr;
		ModifierDefinition const* def = nullptr;
		bool manage_pay = false;
		bool is_payable = false;
	};

	// Internal constructor implementation. Expects _i be expanded to modifier.
	ModifierBlockConverter(
		Context const& _ctx,
		CallState const& _statedata,
		TypeConverter const& _types
	);
};

// -------------------------------------------------------------------------- //

}
}
}
