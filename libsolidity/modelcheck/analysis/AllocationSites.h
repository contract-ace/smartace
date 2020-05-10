/**
 * Provides utilities for identifying call sites for dynamic smart contract
 * allocations. This handles reasoning and validation, through static methods.
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
class NewCallSummary
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
    NewCallSummary(ContractDefinition const& _src);

    // Returns a summary of all children spawned by this contract.
    CallGroup const& children() const;

    // Returns a list of new calls which violate our model. In theory this will
    // be any new call for which an exact bound is not inferred on the number of
    // executions.
    //
    // In this implementation, violations are overapproximated by any new call
    // performed outside of a constructor.
    CallGroup const& violations() const;

private:
    CallGroup m_children;
    CallGroup m_violations;

    // Utility to traverse a function's AST, in order to exact each call to new.
    class Visitor : public ASTConstVisitor
    {
    public:
        CallGroup calls;

        Visitor(FunctionDefinition const* _context, size_t _depth_limit);

    protected:
        bool visit(FunctionCall const& _node) override;
        bool visit(Assignment const& _node) override;
        bool visit(Return const& _node) override;

    private:
        size_t const M_DEPTH_LIMIT;

        FunctionDefinition const* m_context;
        VariableDeclaration const* m_dest = nullptr;
        bool m_return = false;

        std::map<FunctionDefinition const*, ContractDefinition const*> _fcache;

        ContractDefinition const* handle_call_type(FunctionType const& _ftype);
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
class NewCallGraph
{
public:
    using Label = ContractDefinition const*;

    // Records the NewCallSummary of each contract in _src.
    void record(SourceUnit const& _src);

    // Determines the number of contracts allocated by instantiating each
    // contract encountered through record(). If a cycle is detected, finalize()
    // terminates early, and exposes said cycle. After calling finalize(), then
    // record() must not be called again.
    void finalize();

    // Assuming finalize() has been called, returns the cost of a given
    // contract.
    size_t cost_of(Label _vertex) const;

    // Returns all direct children of a contract.
    NewCallSummary::CallGroup const& children_of(Label _vertex) const;

    // Returns all violations found within the graph.
    NewCallSummary::CallGroup const& violations() const;

    // Performs a reverse lookup from contract name to contract address.
    Label reverse_name(std::string _name) const;

    // Returns true if the given return value has been mapped to a new call.
    bool retval_is_allocated(VariableDeclaration const& _decl) const;

    // Returns a more percise contract type for a given contract variable. This
    // takes into account upcasting. Throws if the variable was not recorded, or
    // if it was not of a contract type.
    ContractDefinition const& specialize(VariableDeclaration const& _decl) const;

private:
    using VarTyping = std::map<VariableDeclaration const*, Label>;
    using Graph = std::map<Label, NewCallSummary::CallGroup>;
    using Reach = std::map<Label, size_t>;
    using Alias = std::map<std::string, Label>;

    bool m_finalized = false;

    void analyze(Graph::iterator _neighbourhood);

    Graph m_vertices;
    Reach m_reach;
    Alias m_names;
    VarTyping m_truetypes;
    NewCallSummary::CallGroup m_violations;
};

// -------------------------------------------------------------------------- //

}
}
}
