/**
 * In SmartACE, we statically resolve contract types, to avoid dynamic dispatch.
 * As a part of this analysis we must know the "true" types of all contract
 * variables, and we must know whether or not a contract return value is given
 * as a reference, or allocated on demand.
 * 
 * This file implements (ad-hoc) flow analysis to determine the flow radius of
 * all new calls. This analysis is conservative.
 * 
 * @todo(scottwe): perhaps we could make this analysis more "rigorous". Right
 *                 now the MiniSol dialect require bundles where the assumptions
 *                 of this analysis are met, so it should be fine for now.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <list>
#include <map>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Analyzes a single contract to identify how many "valid" contracts it
 * allocates, along with instances of invalid allocations (under our model).
 */
class AllocationSummary
{
public:
    // Types of violations.
    // - None:          the allocation is "safe"
    // - Orphaned:      the allocation is not assigned to some state variable
    // - Unbounded:     the new operation may be called more than once
    // - TypeConfusion: the allocation has at least two possible types
    enum class ViolationType { None, Orphaned, Unbounded, TypeConfusion };

    // Summarizes a single `new` call.
    struct NewCall
    {
        ContractDefinition const* type;
        FunctionDefinition const* context;
        FunctionCall const* callsite;
        VariableDeclaration const* dest;
        bool is_retval;
        ViolationType status;
    };
    using CallGroup = std::list<NewCall>;

    // Summarizes all allocations across all functions in _src. Only direct
    // children are represented in the summary.
    AllocationSummary(ContractDefinition const& _src);

    // Returns a summary of all children spawned by this contract.
    CallGroup children() const;

    // Returns a list of new calls which violate our model. In theory this will
    // be any new call for which an exact bound is not inferred on the number of
    // executions.
    //
    // In this implementation, violations are overapproximated by any new call
    // performed outside of a constructor.
    CallGroup violations() const;

private:
    CallGroup m_children;
    CallGroup m_violations;

    // Utility to traverse a function's AST, in order to exact each call to new.
    class Visitor : public ASTConstVisitor
    {
    public:
        CallGroup calls;

        // Extracts new calls from _context, up to a call depth of _depth_limit.
        Visitor(
            ContractDefinition const& _src,
            FunctionDefinition const& _context,
            size_t _depth_limit
        );

    protected:
        bool visit(FunctionCall const& _node) override;
        bool visit(Assignment const& _node) override;
        bool visit(Return const& _node) override;

    private:
        size_t const M_DEPTH_LIMIT;

        FunctionDefinition const& m_context;
        ContractDefinition const& m_src;
    
        VariableDeclaration const* m_dest = nullptr;
        bool m_return = false;

        std::map<FunctionDefinition const*, ContractDefinition const*> _fcache;

        // Utility method to handle _ftype based on call type.
        ContractDefinition const* handle_call_type(
            FunctionDefinition const& _call
        );
    };
};

// -------------------------------------------------------------------------- //

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
class AllocationGraph
{
public:
    using Label = ContractDefinition const*;

    // Constructs the allocation graph for _model.
    AllocationGraph(std::vector<ContractDefinition const*> _model);

    // Returns the cost of allocating _vertex.
    size_t cost_of(Label _vertex) const;

    // Returns all direct children of a contract.
    AllocationSummary::CallGroup children_of(Label _vertex) const;

    // Returns all violations found within the graph.
    AllocationSummary::CallGroup violations() const;

    // Returns true if _var has been mapped to a new call.
    bool retval_is_allocated(VariableDeclaration const& _var) const;

    // Returns a more precise contract type for _var. This takes into account
    // upcasting. Throws if the variable was not recorded, or if it was not of a
    // contract type.
    ContractDefinition const& specialize(VariableDeclaration const& _var) const;

    // Returns a more precise contract type for _expr. This takes into account
    // upcasting. Throws if the expression cannot be mapped to a recorded
    // declaration.
    ContractDefinition const& resolve(Expression const& _expr) const;

private:
    using VarTyping = std::map<VariableDeclaration const*, Label>;
    using Graph = std::map<Label, AllocationSummary::CallGroup>;
    using Reach = std::map<Label, size_t>;

    // Computes and caches the cost of constructing each neighbour. Cost is
    // defined as the number of instantiated constracts
    void analyze(Label _root, AllocationSummary::CallGroup _children);

    Graph m_vertices;
    Reach m_reach;
    VarTyping m_truetypes;
    AllocationSummary::CallGroup m_violations;
};

// -------------------------------------------------------------------------- //

}
}
}
