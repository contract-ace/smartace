/**
 * This file implements utilities for traversing the Solidity AST.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <string>
#include <type_traits>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Stringifies unknown types for use in error messages. The error type for _node
 * is returned.
 */
std::string get_error_type(ASTNode const* _node);

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
 * Prunes all no-op syntax from an expression.
 */
class ExpressionCleaner : public ASTConstVisitor
{
public:
	// Cleans _expr.
	ExpressionCleaner(Expression const& _expr);

	// Returns the cleaned expression.
	Expression const& clean() const;

protected:
	ASTNode const* m_res;

	bool visitNode(ASTNode const& _node) override;

	bool visit(TupleExpression const& _node) override;
};

// -------------------------------------------------------------------------- //

/**
 * Returns true if _decl is a storage reference.
 */
bool decl_is_ref(VariableDeclaration const& _decl);

// -------------------------------------------------------------------------- //

/**
 * Follows back _access to a specific field declaration.
 */
VariableDeclaration const* member_access_to_decl(MemberAccess const& _access);

/**
 * Returns a referenced declaration from _exp. On failure, this returns null.
 * Note that this is more general than expr_to_decl. While expr_to_decl must
 * return a variable declaration, this can also return contracts, structs, and
 * function declarations.
 */
Declaration const* node_to_ref(ASTNode const& _node);

/**
 * Consumes _expr and if it is an identifier or member access, the variable
 * declaration is returned. In the case of a magic variable, the nullptr is
 * returned. If expression is none of the above, this throws.
 */
VariableDeclaration const* expr_to_decl(Expression const& _expr);

// -------------------------------------------------------------------------- //

}
}
}
