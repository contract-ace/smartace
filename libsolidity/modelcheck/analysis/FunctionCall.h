/**
 * @date 2019
 * Utilities to extract the parameters of a function call. This includes gas,
 * value, and context.
 */

#pragma once

#include "libsolidity/ast/ASTVisitor.h"
#include "libsolidity/modelcheck/utils/General.h"
#include <list>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class FunctionCallAnalyzer: public ASTConstVisitor
{
public:
    // Extracts relevant data about _call which is not readily available with
    // merely type annotations.
    FunctionCallAnalyzer(FunctionCall const& _call);

    // Accessors.
    std::vector<ASTPointer<Expression const>> const& args() const;
    ASTPointer<Expression const> value() const;
    ASTPointer<Expression const> gas() const;
    Expression const* context() const;
    Identifier const* id() const;

protected:
    bool visit(MemberAccess const& _node) override;
    bool visit(FunctionCall const& _node) override;
    bool visit(Identifier const& _node) override;

private:
    ASTPointer<Expression const> m_last;

    std::vector<ASTPointer<Expression const>> m_args;
    ASTPointer<Expression const> m_value = nullptr;
    ASTPointer<Expression const> m_gas = nullptr;
    Expression const* m_context = nullptr;
    Identifier const* m_id = nullptr;
};

}
}
}
