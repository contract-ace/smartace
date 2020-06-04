/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/utils/CompilerMacros.h>

#include <ostream>
#include <utility>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallState;
class FunctionSpecialization;
class NewCallGraph;
class TypeConverter;

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
		std::vector<ASTPointer<VariableDeclaration>> const& _rvs,
		Block const& _body,
		TypeConverter const& _converter,
		CallState const& _statedata,
		NewCallGraph const& _newcalls,
		bool _manage_pay,
		bool _is_payable
	);

	virtual ~GeneralBlockConverter() = 0;

	// Generates a SimpleCGenerator representation of the Solidity function.
	std::shared_ptr<CBlock> convert();

protected:
	// A taxonomy of block translations.
	// - Initializer: wraps and returns a constructor call
	// - Action: function without a return parameter.
	// - Operation: produces one or more return values.
	enum class BlockType { Initializer, Action, Operation, AddressRef };

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
	CStmtPtr last_substmt();

	// Returns the block type.
	BlockType block_type() const;

	// Returns true if the function has a return value.
	bool has_retval() const;

	bool visit(Block const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(InlineAssembly const&) override;
	bool visit(Throw const& _node) override;
	bool visit(EmitStatement const& _node) override;
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;

	void endVisit(Break const&) override;
	void endVisit(Continue const&) override;

private:
	// Analyzes the return values to classify the block type of this block.
	static BlockType determine_block_type(
		std::vector<ASTPointer<VariableDeclaration>> const& _rvs,
		NewCallGraph const& _newcalls
	);

	// Generates the payment call.
	static void add_value_handler(CBlockList & _block);

	Block const& M_BODY;
	TypeConverter const& M_CONVERTER;
	CallState const& M_STATEDATA;

	bool const M_MANAGE_PAY;
	bool const M_IS_PAYABLE;
	BlockType const M_BLOCKTYPE;

	VariableScopeResolver m_decls;

	CStmtPtr m_substmt;
	std::shared_ptr<CBlock> m_top_block;

	bool m_is_top_level = true;
};

// -------------------------------------------------------------------------- //

// Issue with overding ASTConstVisitor from libsolidity/ast/AST_Visitor.h
CLANG_SUPPRCESS_COMPILER_WARNING_CLANG(CLANG_OVERLOADED_VIRTUAL)

/**
 * Specializes GeneralBlockConverter for FunctionDefinitions, adding support for
 * named and unnamed return values.
 */
class FunctionBlockConverter : public GeneralBlockConverter
{
public:
	// Creates a block converter for _func's main body. It is assumed that
	// _converter is able to resolve all types in the AST of the source
	// unit(s) associated with _func.
	FunctionBlockConverter(
		FunctionDefinition const& _func,
		TypeConverter const& _converter,
		CallState const& _statedata,
		NewCallGraph const& _newcalls
	);

	~FunctionBlockConverter() override = default;

	void set_for(FunctionSpecialization const& _for);

protected:
	void enter(CBlockList & _stmts, VariableScopeResolver & _decls) override;
	void exit(CBlockList & _stmts, VariableScopeResolver & _decls) override;

	bool visit(Return const& _node) override;

private:
	TypeConverter const& M_CONVERTER;

	FunctionSpecialization const* m_spec;

	ASTPointer<VariableDeclaration> m_rv = nullptr;
};

CLANG_ENABLE_COMPILER_WARNING()

// -------------------------------------------------------------------------- //

// Issue with overding ASTConstVisitor from libsolidity/ast/AST_Visitor.h
CLANG_SUPPRCESS_COMPILER_WARNING_CLANG(CLANG_OVERLOADED_VIRTUAL)

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
	// Modifiers introduce two complications. First, modifier invocations are
	// disjoint from their declarations. Second, modifiers are conflated with
	// parent constructors when _func is a constructor. The modifier factory
	// will resolve invocations with definitions, while also filtering away
	// constructor calls.
	class Factory
	{
	public:
		// Preprocesses _func to generate all of its modifiers. _name is the
		// name to associate with _func.
		Factory(FunctionSpecialization const& _spec);

		// Generates the _i-th modifier for _func, where _i is zero-indexed.
		ModifierBlockConverter generate(
			size_t _i,
			TypeConverter const& _converter,
			CallState const& _statedata,
			NewCallGraph const& _newcalls
		) const;

		// Returns the number of modifiers which were not filtered away.
		size_t len() const;

		// Returns true if the function had some modifier, which was not
		// filtered away.
		bool empty() const;
	
	private:
		FunctionSpecialization const& M_SPEC;

		std::vector<
			std::pair<ModifierDefinition const*, ModifierInvocation const*>
		> m_filtered_mods;
	};

	~ModifierBlockConverter() override = default;

protected:
	void enter(CBlockList & _stmts, VariableScopeResolver & _decls) override;
	void exit(CBlockList & _stmts, VariableScopeResolver &) override;

	bool visit(Return const&) override;

	void endVisit(PlaceholderStatement const&) override;

private:
	TypeConverter const& M_CONVERTER;
	CallState const& M_STATEDATA;
	std::vector<ASTPointer<VariableDeclaration>> const& M_TRUE_PARAMS;
	std::vector<ASTPointer<VariableDeclaration>> const& M_USER_PARAMS;
	std::vector<ASTPointer<Expression>> const* M_USER_ARGS;
	std::string const M_NEXT_CALL;

	VariableScopeResolver m_shadow_decls;
	std::shared_ptr<CVarDecl> m_rv = nullptr;

	// Internal constructor implementation. Expects _i be expanded to modifier.
	ModifierBlockConverter(
		FunctionDefinition const& _func,
		ModifierDefinition const* _def,
		ModifierInvocation const* _curr,
		TypeConverter const& _converter,
		CallState const& _statedata,
		NewCallGraph const& _newcalls,
		std::string _next,
		bool _entry
	);
};

CLANG_ENABLE_COMPILER_WARNING()

// -------------------------------------------------------------------------- //

}
}
}
