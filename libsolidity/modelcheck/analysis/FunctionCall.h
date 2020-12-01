/**
 * Utilities to extract the parameters of a function call. This includes gas,
 * value, and context.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utilities to extract the parameters of function calls.
 */
class FunctionCallAnalyzer: public ASTConstVisitor
{
public:
    // Classifies all calls, as they are handled by SmartACE.
    enum class CallGroup
    {
        Method, Delegate, Constructor, Send, Transfer, Crypto, Destruct, Revert,
        Assert, Require, Logging, Blockhash, AddMod, MulMod, Push, Pop,
        NewArray, ABI, GasLeft, NoOp, UnhandledCall
    };

    // Extracts relevant data about _call which is not readily available with
    // merely type annotations.
    FunctionCallAnalyzer(FunctionCall const& _call);

    // Returns the list of arguments to the method.
    std::vector<ASTPointer<Expression const>> const& args() const;

    // Returns the value passed to the method, if provided.
    ASTPointer<Expression const> value() const;

    // Returns the gas passed to the method, if provided.
    ASTPointer<Expression const> gas() const;

    // Returns the context used to invoke the method, if provided.
    Expression const* context() const;

    // If true, this method is a call to a super method.
    bool is_super() const;

    // If true, this is a library call.
    bool is_in_library() const;

    // If true, this is a low-level operation through `.call`.
    bool is_low_level() const;

    // If true, the call is a remote call to "this".
    bool context_is_this() const;

    // Returns the type metadata for the underlying method.
    FunctionType const& type() const;

    // Returns the declaration of the method being called.
    FunctionDefinition const& decl() const;

    // Classifies the method into a group handled by SmartACE.
    CallGroup classify() const;

protected:
    bool visit(MemberAccess const& _node) override;
    bool visit(FunctionCall const& _node) override;
    bool visit(Identifier const& _node) override;

private:
    ASTPointer<Expression const> m_last;

    std::vector<ASTPointer<Expression const>> m_args;
    FunctionTypePointer m_type;

    FunctionDefinition const* m_decl = nullptr;

    ASTPointer<Expression const> m_value = nullptr;
    ASTPointer<Expression const> m_gas = nullptr;
    Expression const* m_context = nullptr;
    Identifier const* m_root = nullptr;

    bool m_low_level = false;
};

// -------------------------------------------------------------------------- //

}
}
}
