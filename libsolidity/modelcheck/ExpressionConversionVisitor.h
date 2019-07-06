/**
 * @date 2019
 * Utility visitor to convert Solidity expressions into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <libsolidity/modelcheck/VariableScopeResolver.h>
#include <array>
#include <functional>
#include <ostream>
#include <utility>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utility to find top-most MemberAccess within an expression AST.
 */
class MemberAccessSniffer : public ASTConstVisitor
{
public:
	// Wraps an AST node from which the member access is located.
	MemberAccessSniffer(
        Expression const& _expr
	);

	// Returns the member accessor if possible, or nullptr.
	MemberAccess const* find();

protected:
	bool visit(MemberAccess const& _node);

private:
    Expression const* m_expr;
    MemberAccess const* m_ret;
};

// -------------------------------------------------------------------------- //

/**
 * A utility visitor, designed to convert Solidity statements into executable
 * C code. This is meant to be used a utility when converting a full Solidity
 * block. Issues such as type inference are handled at this level.
 */
class ExpressionConversionVisitor : public ASTConstVisitor
{
public:
    // Creates a visitor, which evaluates an expressive, given a fixed scope and
	// declaration set. Will also propogate and expose relevant expression data.
    ExpressionConversionVisitor(
        Expression const& _expr,
        TypeTranslator const& _scope,
        VariableScopeResolver const& _decls
    );

    // Generates a human-readable block of C code, from the given expression.
    void print(std::ostream& _stream);

protected:
	bool visit(EnumValue const& _node) override;
	bool visit(ModifierInvocation const& _node) override;
	bool visit(Conditional const& _node) override;
	bool visit(Assignment const& _node) override;
	bool visit(TupleExpression const& _node) override;
	bool visit(UnaryOperation const& _node) override;
	bool visit(BinaryOperation const& _node) override;
	bool visit(FunctionCall const& _node) override;
	bool visit(NewExpression const& _node) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(IndexAccess const& _node) override;
	bool visit(Identifier const& _node) override;
	bool visit(ElementaryTypeNameExpression const& _node) override;
	bool visit(Literal const& _node) override;

private:
    Expression const* m_expr;
    TypeTranslator const& m_scope;
	VariableScopeResolver const& m_decls;

	std::ostream* m_ostream = nullptr;

	std::hash<std::string> m_hasher;

	static std::map<std::pair<MagicType::Kind, std::string>, std::string> const m_magic_members;

	static long long int literal_to_number(Literal const& _node);

	void print_subexpression(Expression const& _node);

	void print_assertion(std::string type, FunctionCall const& _func);
	void print_payment(FunctionCall const& _func);
	void print_ext_method(FunctionType const& _type, FunctionCall const& _func);
	void print_method(
		FunctionType const& _type,
		Expression const& _ctx,
		std::vector<ASTPointer<Expression const>> const& _args);

	void print_address_member(
		Expression const& _node, std::string const& _member);
	void print_array_member(
		Expression const& _node, std::string const& _member);
	void print_adt_member(Expression const& _node, std::string const& _member);
	void print_magic_member(TypePointer _type, std::string const& _member);
};

// -------------------------------------------------------------------------- //

}
}
}
