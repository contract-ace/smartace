/**
 * @date 2019
 * Utility visitor to convert Solidity expressions into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/SimpleCCore.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <libsolidity/modelcheck/VariableScopeResolver.h>
#include <map>
#include <string>
#include <vector>
#include <type_traits>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utility to find the top-most instance of T along some branch of an
 * expression AST. Sub-expressions used in decisions shell be pruned from the
 * tree.
 */
template <class T>
class NodeSniffer : public ASTConstVisitor
{
public:
	static_assert(std::is_base_of<ASTNode, T>::value);

	// Wraps an AST node from which a node of type T is located.
	NodeSniffer(Expression const& _expr): m_expr(_expr) {}

	// Returns the node of type T if possible, or nullptr.
	T const* find()
	{
		m_ret = nullptr;
		m_expr.accept(*this);
		return m_ret;
	}

protected:
	bool visit(Conditional const& _node) override
	{
		_node.falseExpression().accept(*this);
		_node.trueExpression().accept(*this);
		return false;
	}

	bool visit(IndexAccess const& _node) override
	{
		_node.baseExpression().accept(*this);
		return false;
	}

	bool visit(FunctionCall const& _node) override
	{
		_node.expression().accept(*this);
		return false;
	}

	void endVisit(T const& _node) override { m_ret = &_node; }

private:
    Expression const& m_expr;
    T const* m_ret;
};

// -------------------------------------------------------------------------- //

/**
 * This utility will consume an expression node. If this expression node
 * contains a potential LVal, it will be located, and if it is of type T, it
 * will be returned.
 */
template <class T>
class LValueSniffer : public ASTConstVisitor
{
public:
	static_assert(
		std::is_same<T, MemberAccess>::value ||
		std::is_same<T, IndexAccess>::value ||
		std::is_same<T, Identifier>::value
	);

	// Wraps an AST node from which the LValue is located.
	LValueSniffer(Expression const& _expr): m_expr(_expr) {}

	// Returns the LValue of type T if possible, or nullptr.
	T const* find()
	{
		m_ret = nullptr;
		m_expr.accept(*this);
		return m_ret;
	}

protected:
	bool visit(Conditional const& _node) override
	{
		_node.falseExpression().accept(*this);
		_node.trueExpression().accept(*this);
		return false;
	}

	bool visit(FunctionCall const&) override { return false; }
	bool visit(MemberAccess const&) override { return false; }
	bool visit(IndexAccess const&) override { return false; }
	bool visit(Identifier const&) override { return false; }

	void endVisit(T const& _node) override { m_ret = &_node; }

private:
	Expression const& m_expr;
	T const* m_ret;
};

// -------------------------------------------------------------------------- //

using SolArgList = std::vector<ASTPointer<Expression const>>;

/**
 * A utility visitor, designed to convert Solidity statements into executable
 * C code. This is meant to be used a utility when converting a full Solidity
 * block. Issues such as type inference are handled at this level.
 */
class ExpressionConverter : public ASTConstVisitor
{
public:
    // Creates a visitor, which evaluates an expressive, given a fixed scope and
	// declaration set. Will also propogate and expose relevant expression data.
    ExpressionConverter(
        Expression const& _expr,
        TypeConverter const& _converter,
        VariableScopeResolver const& _decls,
		bool _is_ref = false
    );

    // Generates a SimpleCGenerator representation of the expression.
    CExprPtr convert();

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
	bool visit(Literal const& _node) override;

private:
    Expression const* M_EXPR;
    TypeConverter const& M_TYPES;
	VariableScopeResolver const& m_decls;

	CExprPtr m_subexpr;

	bool m_lval = false;
	bool m_find_ref;

	static std::map<std::pair<MagicType::Kind, std::string>, std::string> const
		m_magic_members;

	// In the present model, all primitives are reduced to integers. This map
	// produces such integers from Solidity literals.
	static long long int literal_to_number(Literal const& _node);

	// Helper functions to format mapping operations.
	void generate_binary_op(
		Expression const& _lhs,
		Token _op,
		Expression const& _rhs
	);
	void generate_mapping_call(
		std::string const& _op,
		std::string const& _id,
		IndexAccess const& _map,
		CExprPtr _v
	);

	// Helper functions to produce specialized function calls.
	void print_struct_ctor(FunctionCall const& _call);
	void print_cast(FunctionCall const& _call);
	void print_function(Expression const& _call, SolArgList const& _args);
	void print_method(
		FunctionType const& _type,
		Expression const* _ctx,
		SolArgList const& _args
	);
	void print_ext_method(
		FunctionType const& _type,
		Expression const& _call,
		SolArgList const& _args
	);
	void print_contract_ctor(Expression const& _call, SolArgList const& _args);
	void print_payment(Expression const& _call, SolArgList const& _args);
	void print_assertion(std::string _type, SolArgList const& _args);

	// Helpe functions to handle certain member access cases.
	void print_address_member(
		Expression const& _node, std::string const& _member
	);
	void print_array_member(
		Expression const& _node, std::string const& _member
	);
	void print_adt_member(
		Type const& _type, Expression const& _node, std::string const& _member
	);
	void print_magic_member(TypePointer _type, std::string const& _member);
};

// -------------------------------------------------------------------------- //

}
}
}
