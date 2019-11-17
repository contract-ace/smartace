/**
 * Provides utilities for identifying call sites for dynamic smart contract
 * allocations. This handles reasoning and validation, through static methods.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <list>
#include <map>
#include <set>

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
    using Children = std::list<ChildType>;

    // Summarizes a single `new` call.
    struct NewCall
    {
        ContractDefinition const* type;
        FunctionDefinition const* context;
        FunctionCall const* callsite;
    };
    using CallGroup = std::list<NewCall>;

    // Summarizes all allocations across all functions in _src. Only direct
    // children are represented in the summary.
    NewCallSummary(ContractDefinition const& _src);

    // Returns a summary of all children spawned by this contract.
    const Children & children() const;

    // Returns a list of new calls which violate our model. In theory this will
    // be any new call for which an exact bound is not inferred on the number of
    // executions.
    //
    // In this implementation, violations are overapproximated by any new call
    // performed outside of a constructor.
    CallGroup const& violations() const;

private:
    Children m_children;
    CallGroup m_violations;

    // Utility to traverse a function's AST, in order to exact each call to new.
    class Visitor : public ASTConstVisitor
    {
    public:
        CallGroup calls;

        Visitor(FunctionDefinition const* _context);

    protected:
        bool visit(FunctionCall const& _node) override;

    private:
        FunctionDefinition const* m_context;
    };
};

/**
 * Analyzes the inter-contract relations between new calls. This is presented as
 * a graph, in which each contract (vertex) is annotated with the number of
 * contracts indirectly allocated.
 * 
 * Indirect allocation can be thought in terms of descendants. If contract D has
 * no children, contract C has two D children, contract B has one D child and
 * two C children, and contract A has one B child and one D child, then contract
 * A has 10 descendants. Note that each contract is considered to be its own
 * descendant.
 */
class NewCallGraph
{
public:
    // Records the NewCallSummary of each contract in _src.
    void record(SourceUnit const& _src);

    // Determines the number of contracts allocated by instantiating each
    // contract encountered through record(). If a cycle is detected, finalize()
    // terminates early, and exposes said cycle. After calling finalize(), then
    // record() must not be called again.
    void finalize();

    // Assuming finalize has been called, returns the cost of a given contract.
    size_t cost_of(ContractDefinition const* _vertex) const;

    // Returns all violations found within the graph.
    NewCallSummary::CallGroup violations() const;

private:
    using Graph = std::map<ContractDefinition const*, NewCallSummary::Children>;

    bool m_finalized = false;

    void analyze(Graph::iterator _neighbourhood);

    Graph m_vertices;
    std::map<ContractDefinition const*, size_t> m_costs;
    NewCallSummary::CallGroup m_violations;
};

}
}
}
