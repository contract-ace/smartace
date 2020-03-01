/**
 * @date 2019
 * Utility visitor to convert Solidity expressions into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Core.h>
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
		CallState const& _statedata,
        TypeConverter const& _converter,
        VariableScopeResolver const& _decls,
		bool _is_ref = false
    );

    // Generates a SimpleCGenerator representation of the expression.
    CExprPtr convert();

protected:
	bool visit(Conditional const& _node) override;
	bool visit(Assignment const& _node) override;
	bool visit(TupleExpression const& _node) override;
	bool visit(UnaryOperation const& _node) override;
	bool visit(BinaryOperation const& _node) override;
	bool visit(FunctionCall const& _node) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(IndexAccess const& _node) override;
	bool visit(Identifier const& _node) override;
	bool visit(Literal const& _node) override;

private:
    Expression const* M_EXPR;
	CallState const& m_statedata;
    TypeConverter const& M_TYPES;
	VariableScopeResolver const& m_decls;

	CExprPtr m_subexpr;
	Identifier const* m_last_assignment;

	bool m_is_address_cast = false;

	bool m_lval = false;
	bool m_find_ref;

	// In the present model, all primitives are reduced to integers. This map
	// produces such integers from Solidity literals.
	static long long int literal_to_number(Literal const& _node);

	// Helper to format binary calls. Unlike unary calls, binary calls appear in
	// multiple cases.
	void generate_binary_op(
		Expression const& _lhs,
		Token _op,
		Expression const& _rhs
	);

	// Helper to format mapping operations.
	void generate_mapping_call(
		std::string const& _op,
		MapDeflate::FlatMap const& _map,
		FlatIndex const& _idx,
		CExprPtr _v
	);

	// Returns the correct context for initializer applications. In a
	// assignment subexpression, the correct location is the LHS. When
	// used recursively in another initializer, the correct location
	// is the `dest` parameter.
	CExprPtr get_initializer_context() const;

	// Helper functions to produce specialized function calls.
	void print_struct_ctor(FunctionCall const& _call);
	void print_cast(FunctionCall const& _call);
	void print_function(FunctionCall const& _call);
	void print_method(FunctionCallAnalyzer const& _calldata);
	void print_contract_ctor(FunctionCall const& _call);
	void print_payment(FunctionCall const& _call, bool _nothrow);
	void print_assertion(std::string _type, SolArgList const& _args);
	void print_revert(SolArgList const&);
	void pass_next_call_state(
		FunctionCallAnalyzer const& _call,
		CFuncCallBuilder & _builder,
		bool _is_ext
	);

	// Helpe functions to handle certain member access cases.
	void print_address_member(
		Expression const& _node, std::string const& _member
	);
	void print_array_member(
		Expression const& _node, std::string const& _member
	);
	void print_adt_member(Expression const& _node, std::string const& _member);
	void print_magic_member(TypePointer _type, std::string const& _member);
	void print_enum_member(TypePointer _type, std::string const& _member);
};

// -------------------------------------------------------------------------- //

}
}
}
