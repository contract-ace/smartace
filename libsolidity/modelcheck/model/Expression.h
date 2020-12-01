/**
 * Converter from Solidity expressions to SmartACE expressions.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/codegen/Details.h>

#include <memory>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AnalysisStack;
class CFuncCallBuilder;
class FunctionCallAnalyzer;
class VariableScopeResolver;

// -------------------------------------------------------------------------- //

using SolArgList = std::vector<ASTPointer<Expression const>>;
using SolDeclList = std::vector<ASTPointer<VariableDeclaration>>;

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
        std::shared_ptr<AnalysisStack const> _stack,
        VariableScopeResolver const& _decls,
		bool _is_ref = false,
		bool _is_init = false
    );

    // Generates a SimpleCGenerator representation of the expression.
    CExprPtr convert();

	// Sets the auxilary rv's for the first tuple-valued function.
	void set_aux_rvs(std::vector<CExprPtr> _rvs);

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
	VariableScopeResolver const& M_DECLS;

	std::vector<CExprPtr> m_aux_rvs;

	std::shared_ptr<AnalysisStack const> m_stack;

	CExprPtr m_subexpr;
	Identifier const* m_last_assignment;

	bool m_is_init;

	bool m_is_address_cast = false;

	bool m_lval = false;
	bool m_find_ref;

	// In the present model, all primitives are reduced to integers. This map
	// produces such integers from Solidity literals.
	static long long int literal_to_number(Literal const& _node);

	// Helper to format binary calls. Unlike unary calls, binary calls appear in
	// multiple cases.
	void generate_binary_op(
		Expression const& _lhs, Token _op, Expression const& _rhs
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

	// Assumes (_args[_offset+i], _decls[i]) gives the i-th argument of some
	// Solidity method, and the declaration it is passed into. All such pairs
	// are added to _builder.
	void push_arglist(
		SolArgList const& _args,
		SolDeclList const& _decls,
		CFuncCallBuilder & _builder,
		size_t _offset = 0
	) const;

	// Assumes (_arg, _decl) gives the argument of some Solidity method, and the
	// declaration it is passed into. The pair is added to _builder.
	void push_arg(
		Expression const& _arg,
		VariableDeclaration const& _decl,
		CFuncCallBuilder & _builder
	) const;

	// Helper functions to produce specialized function calls.
	void print_struct_ctor(FunctionCall const& _call);
	void print_cast(FunctionCall const& _call);
	void print_function(FunctionCall const& _call);
	void print_method(FunctionCallAnalyzer const& _calldata);
	void print_contract_ctor(FunctionCall const& _call);
	void print_payment(FunctionCall const& _call, bool _nothrow);
	void print_call(FunctionCallAnalyzer const& _call);
	void print_require(CExprPtr _expr, std::string const& _msg);
	void print_revert();
	void print_property(bool _fail, SolArgList const& _args);
	void pass_next_call_state(
		FunctionCallAnalyzer const& _call,
		CFuncCallBuilder & _builder,
		bool _is_ext
	);

	// Helper functions to handle certain member access cases.
	void print_address_member(
		Expression const& _node, std::string const& _member
	);
	void print_array_member(
		Expression const& _node, std::string const& _member
	);
	void print_adt_member(Expression const& _node, std::string _member);
	void print_magic_member(TypePointer _t, std::string _member);
	void print_enum_member(TypePointer _t, std::string const& _val);
};

// -------------------------------------------------------------------------- //

}
}
}
