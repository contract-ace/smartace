/**
 * Provides utilities for identifying call sites for dynamic smart contract
 * allocations. This handles reasoning and validation, through static methods.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <list>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Analyzes a single contract to identify how many "valid" contracts it
 * allocates, along with instances of invalid allocations (under our model).
 */
class NewCallSummary
{
public:
    // Aggregates all `new` calls to a single type.
    struct ChildType
    {
        ContractDefinition const* type;
        size_t count;
    };

    // Summarizes a single `new` call.
    struct NewCall
    {
        ContractDefinition const* type;
        FunctionDefinition const* context;
        FunctionCall const* callsite;
    };

    // Summarizes all allocations across all functions in _src. Only direct
    // children are represented in the summary.
    NewCallSummary(ContractDefinition const& _src);

    // Returns a summary of all children spawned by this contract.
    const std::list<ChildType> & children() const;

    // Returns a list of new calls which violate our model. In theory this will
    // be any new call for which an exact bound is not inferred on the number of
    // executions.
    //
    // In this implementation, violations are overapproximated by any new call
    // performed outside of a constructor.
    std::list<NewCall> const& violations() const;

private:
    std::list<ChildType> m_children;
    std::list<NewCall> m_violations;

    // Utility to traverse a function's AST, in order to exact each call to new.
    class Visitor : public ASTConstVisitor
    {
    public:
        std::list<NewCall> calls;

        Visitor(FunctionDefinition const* _context);

    protected:
        bool visit(FunctionCall const& _node) override;

    private:
        FunctionDefinition const* m_context;
    };
};

}
}
}
