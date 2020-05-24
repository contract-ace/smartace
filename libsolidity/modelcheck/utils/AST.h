/**
 * @date 2019
 * This file operators utilities for traversing the Solidity AST. This is a
 * header-only library, consisting of simple, generative utilities.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <type_traits>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * This searches the inheritance hierarchy of _base to find instances of T with
 * the name _target. The top instance is returned. If no matches are found, the
 * nullptr is returned.
 */
template <class T>
T const* find_named_match(
	ContractDefinition const* _base, std::string const& _target
)
{
	for (auto contract : _base->annotation().linearizedBaseContracts)
	{
		for (auto callable : ASTNode::filteredNodes<T>(contract->subNodes()))
		{
			if (callable->name() == _target) return callable;
		}
	}
	return nullptr;
}

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
	static_assert(
		std::is_base_of<ASTNode, T>::value,
		"NodeSniffer expects that T be a valid ASTNode type."
	);

	// Wraps an AST node from which a node of type T is located.
	NodeSniffer(Expression const& _expr, bool _use_func_args = false)
	: M_EXPR(_expr), M_USE_FUNC_ARGS(_use_func_args) {}

	// Returns the node of type T if possible, or nullptr.
	T const* find()
	{
		m_ret = nullptr;
		M_EXPR.accept(*this);
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
		if (!M_USE_FUNC_ARGS) _node.expression().accept(*this);
		return M_USE_FUNC_ARGS;
	}

	void endVisit(T const& _node) override { m_ret = &_node; }

private:
    Expression const& M_EXPR;
	bool const M_USE_FUNC_ARGS;
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
		std::is_same<T, MemberAccess>::value
			|| std::is_same<T, IndexAccess>::value
			|| std::is_same<T, Identifier>::value,
		"LValue extraction is only defined on members, indices or identifiers"
	);

	// Wraps an AST node from which the LValue is located.
	LValueSniffer(Expression const& _expr): M_EXPR(_expr) {}

	// Returns the LValue of type T if possible, or nullptr.
	T const* find()
	{
		m_ret = nullptr;
		M_EXPR.accept(*this);
		return m_ret;
	}

protected:
	bool visit(Conditional const& _node) override
	{
		_node.falseExpression().accept(*this);
		_node.trueExpression().accept(*this);
		return false;
	}

	bool visit(Assignment const&_node) override
	{
		_node.leftHandSide().accept(*this);
		return false;
	}

	bool visit(FunctionCall const&) override { return false; }
	bool visit(MemberAccess const&) override { return false; }
	bool visit(IndexAccess const&) override { return false; }
	bool visit(Identifier const&) override { return false; }

	void endVisit(T const& _node) override { m_ret = &_node; }

private:
	Expression const& M_EXPR;
	T const* m_ret;
};

// -------------------------------------------------------------------------- //

/**
 * Follows back a member access for a specific field declaration.
 */
VariableDeclaration const* member_access_to_decl(MemberAccess const& _access);

// -------------------------------------------------------------------------- //

}
}
}
