/**
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <libsolidity/modelcheck/utils/Function.h>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AllocationGraph;

// -------------------------------------------------------------------------- //

/**
 * A utility class which extracts all calls made by invoking a given function.
 */
class CallReachAnalyzer: public ASTConstVisitor
{
public:
    // Determines all calls originating from the body of _func.
    CallReachAnalyzer(FunctionDefinition const& _func);

    std::set<FunctionDefinition const*> m_calls;
    std::set<VariableDeclaration const*> m_reads;

protected:
    bool visit(IndexAccess const& _node) override;

    void endVisit(FunctionCall const& _node) override;
};

// -------------------------------------------------------------------------- //

/**
 * The contract dependance is a second pass over the contract construction
 * graph. It is compared against a model (a list of contracts to model) and then
 * uses these contracts to determine the structures and methods we require to
 * resolve all calls.
 */
class ContractDependance
{
public:
    using ContractSet = std::set<ContractDefinition const*>;
    using ContractList = std::vector<ContractDefinition const*>;
    using FunctionSet = std::set<FunctionDefinition const*>;
    using FuncInterface = std::list<FunctionDefinition const*>;
    using VarSet = std::set<VariableDeclaration const*>;

    // A utility used by ContractDependance to expand the entire model. The
    // DependanceAnalyzer handles targeted analysis without concern for how each
    // component will be stitched together by the ContractDependance structure.
    class DependencyAnalyzer
    {
    public:
        // The _model parameter is needed for non-test setups, to list top level
        // contracts in the scheduler.
        DependencyAnalyzer(ContractDependance::ContractList _model);

        virtual ~DependencyAnalyzer() = default;

        // Returns all methods exposed (and used) by _contract.
        virtual FuncInterface get_interfaces_for(
            ContractDefinition const* _contract
        ) const = 0;

        // Returns the super call chain for _func, as taken from the interface
        // list _interfaces.
        virtual FunctionSet get_superchain_for(
            ContractDependance::FuncInterface _interfaces,
            FunctionDefinition const* _func
        ) const = 0;

        // The list of all contracts in the analysis.
        ContractDependance::ContractSet m_contracts;

        // The list of all contracts specified by the model.
        ContractDependance::ContractList m_model;
    };

    // Default constructor used to orchestrate dependency analysis.
    ContractDependance(DependencyAnalyzer const& _analyzer);

    // Returns all top level contracts in the graph, given the graph is meant
    // to generate a scheduler.
    ContractList const& get_model() const;

    // Returns all methods in the graph. This includes methods which are called
    // indirectly (i.e., as a call to super).
    FunctionSet const& get_executed_code() const;

    // Returns true if the contract is ever used.
    bool is_deployed(ContractDefinition const* _actor) const;

    // Returns the public method of a contract.
    FuncInterface const& get_interface(ContractDefinition const* _actor) const;

    // Returns all super calls for a given method.
    FunctionSet const& get_superchain(
        ContractDefinition const* _contract, FunctionDefinition const* _func
    ) const;

    // Returns all methods invoked by this call.
    FunctionSet const& get_function_roi(FunctionDefinition const* _func) const;

    // Returns all mapping declarations touched by a given function.
    VarSet const& get_map_roi(FunctionDefinition const* _func) const;

private:
    // Stores interfaces and inheritance chains, with respect to a given
    // contract.
    struct ContractRecord
    {
        FuncInterface interfaces;
        std::map<FunctionDefinition const*, FunctionSet> superchain;
    };

    ContractList m_model;
    FunctionSet m_functions;

    std::map<ContractDefinition const*, ContractRecord> m_function_data;

    std::map<FunctionDefinition const*, FunctionSet> m_callreach;
    std::map<FunctionDefinition const*, VarSet> m_mapreach;
};

// -------------------------------------------------------------------------- //


/**
 * An implementation of DependencyAnalyzer which expands all calls. This is
 * meant for codegen testing.
 */
class FullSourceContractDependance
    : public ContractDependance::DependencyAnalyzer
{
public:
    // All contracts reachable for _srcs are included.
    FullSourceContractDependance(SourceUnit const& _srcs);

    ~FullSourceContractDependance() override = default;

    ContractDependance::FuncInterface get_interfaces_for(
        ContractDefinition const* _contract
    ) const override;

    ContractDependance::FunctionSet get_superchain_for(
        ContractDependance::FuncInterface _interface,
        FunctionDefinition const* _func
    ) const override;
};

// -------------------------------------------------------------------------- //

/**
 * An implementation of DependencyAnalyzer which expands only the calls needed
 * by a given model, with a given allocation graph.
 */
class ModelDrivenContractDependance
    : public ContractDependance::DependencyAnalyzer
{
public:
    // All contracts reachable from _model, taking into account downcasting in
    // _graph, are included.
    ModelDrivenContractDependance(
        std::vector<ContractDefinition const*> _model,
        AllocationGraph const& _graph
    );

    ~ModelDrivenContractDependance() override = default;

    ContractDependance::FuncInterface get_interfaces_for(
        ContractDefinition const* _contract
    ) const override;

    ContractDependance::FunctionSet get_superchain_for(
        ContractDependance::FuncInterface _interfaces,
        FunctionDefinition const* _func
    ) const override;

private:
    // Utility class used to extract the actual chain of super calls.
    class SuperChainExtractor : public ASTConstVisitor
    {
    public:
        SuperChainExtractor(FunctionDefinition const& _call);
        ContractDependance::FunctionSet m_superchain;
        FunctionSpecialization m_base;

    protected:
	    void endVisit(FunctionCall const& _node) override;
    };
};

// -------------------------------------------------------------------------- //

}
}
}
