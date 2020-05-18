/**
 * @date 2020
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>

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
    using FuncInterface = std::list<FunctionDefinition const*>;
    using SuperCalls = std::vector<FunctionDefinition const*>;

    // A utility used by ContractDependance to expand the entire model. The
    // DependanceAnalyzer handles targeted analysis without concern for how each
    // component will be stitched together by the ContractDependance structure.
    class DependancyAnalyzer
    {
    public:
        virtual ~DependancyAnalyzer() = default;

        // Returns all methods exposed (and used) by _ctrt.
        virtual FuncInterface get_interfaces_for(
            ContractDefinition const* _ctrt
        ) const = 0;

        // Returns the super call chain for _func.
        virtual SuperCalls get_superchain_for(
            FunctionDefinition const* _func
        ) const = 0;

        // The list of all contracts in the model.
        ContractDependance::ContractSet m_contracts;
    };

    // Default constructor used to orchestrate dependancy analysis.
    ContractDependance(DependancyAnalyzer const& _analyzer);

    // Returns true if the contract is ever used.
    bool is_deployed(ContractDefinition const* _actor) const;

    // Returns the public method of a contract.
    FuncInterface const& get_interface(ContractDefinition const* _actor) const;

    // Returns all super calls for a given method.
    SuperCalls const& get_superchain(FunctionDefinition const* _func) const;

private:
    ContractSet m_contracts;

    std::map<ContractDefinition const*, FuncInterface> m_interfaces;

    std::map<FunctionDefinition const*, SuperCalls> m_superchain;
};

// -------------------------------------------------------------------------- //


/**
 * An implementation of DependancyAnalyzer which expands all calls. This is
 * meant for codegen testing.
 */
class FullSourceContractDependance
    : public ContractDependance::DependancyAnalyzer
{
public:
    // All contracts reachable for _srcs are included.
    FullSourceContractDependance(SourceUnit const& _srcs);

    ~FullSourceContractDependance() override = default;

    ContractDependance::FuncInterface get_interfaces_for(
        ContractDefinition const* _ctrt
    ) const override;

    ContractDependance::SuperCalls get_superchain_for(
        FunctionDefinition const* _func
    ) const override;
};

// -------------------------------------------------------------------------- //

/**
 * An implementation of DependancyAnalyzer which expands only the calls needed
 * by a given model, with a given allocation graph.
 */
class ModelDrivenContractDependance
    : public ContractDependance::DependancyAnalyzer
{
public:
    // All contracts reachable from _model, taking into account downcasting in
    // _graph, are included.
    ModelDrivenContractDependance(
        std::vector<ContractDefinition const*> _model,
        NewCallGraph const& _graph
    );

    ~ModelDrivenContractDependance() override = default;

    ContractDependance::FuncInterface get_interfaces_for(
        ContractDefinition const* _ctrt
    ) const override;

    ContractDependance::SuperCalls get_superchain_for(
        FunctionDefinition const* _func
    ) const override;

private:
    // Utility class used to extract the actual chain of super calls.
    class SuperChainExtractor : public ASTConstVisitor
    {
    public:
        SuperChainExtractor(FunctionDefinition const& _call);

        ContractDependance::SuperCalls m_superchain;

    protected:
	    bool visit(FunctionCall const& _node) override;
    };
};

// -------------------------------------------------------------------------- //

}
}
}
