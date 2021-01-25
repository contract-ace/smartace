/**
 * SmartACE relies on several full source analyses which are populated through
 * the entire translation. This file provides tools to set up these analyses,
 * and then easily pass their results through the model.
 * 
 * @date 2020
 */

#pragma once

#include <memory>
#include <vector>

namespace dev
{
namespace solidity
{
class ContractDefinition;
class SourceUnit;
}
}

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AllocationGraph;
class CallGraph;
class CallState;
class ContractExpressionAnalyzer;
class FlatModel;
class LibrarySummary;
class MapIndexSummary;
class TightBundleModel;
class TypeAnalyzer;

using InheritanceModel = std::vector<ContractDefinition const*>;

// -------------------------------------------------------------------------- //

/**
 * The first pass of analysis devirtualizes contracts.
 */
class AllocationAnalysis
{
public:
    // Equivalent to AllocationGraph(_model), with some additional error
    // handling and post analysis summary.
    AllocationAnalysis(InheritanceModel const& _model);

    // Describes all virtual allocations in the bundle.
    std::shared_ptr<AllocationGraph const> allocations() const;

private:
    std::shared_ptr<AllocationGraph> m_allocation_graph;
};

// -------------------------------------------------------------------------- //

/**
 * The second pass of analysis uses the devirtualization graph to build a model
 * free from hierarchies.
 */
class InheritanceAnalysis : public AllocationAnalysis
{
public:
    // Equivalent to calling FlatModel(_model, *allocations()).
    InheritanceAnalysis(InheritanceModel const& _model);

    // Describes all contracts in the model.
    std::shared_ptr<FlatModel const> model() const;

private:
    std::shared_ptr<FlatModel> m_flat_model;
};

// -------------------------------------------------------------------------- //

/**
 * The third pass of analysis assigns unique identifiers to each contract
 * instance. This is different from FlatContracts. There may be two or more
 * instances of a single flat contract in a tightly coupled smart contract
 * bundle.
 */
class TightBundleAnalysis : public InheritanceAnalysis
{
public:
    // Equivalent to TightBundleModel(*model()).
    TightBundleAnalysis(InheritanceModel const& _model);

    // A unique identity for each contract instance.
    std::shared_ptr<TightBundleModel const> tight_bundle() const;

private:
    std::shared_ptr<TightBundleModel> m_tight_bundle;
};

// -------------------------------------------------------------------------- //

/**
 * The third pass of analysis upcasts all contract return values. The end result
 * is a mapping from contract expressions to upcast contracts.
 */
class ContractExprAnalysis : public TightBundleAnalysis
{
public:
    // Equivalent to ContractExpressionAnalyzer(*model(), *allocations()).
    ContractExprAnalysis(InheritanceModel const& _model);

    // Describes all calls made within the bundle.
    std::shared_ptr<ContractExpressionAnalyzer const> contracts() const;

private:
    std::shared_ptr<ContractExpressionAnalyzer> m_contracts;
};

// -------------------------------------------------------------------------- //

/**
 * The fourth pass of analysis constructs a call graph for all contracts in the
 * model.
 */
class FlatCallAnalysis : public ContractExprAnalysis
{
public:
    // Equivalent to calling CallGraph(*model(), *allocations()).
    FlatCallAnalysis(InheritanceModel const& _model);

    // Describes all calls made within the bundle.
    std::shared_ptr<CallGraph const> calls() const;

private:
    std::shared_ptr<CallGraph> m_call_graph;
};

// -------------------------------------------------------------------------- //

/**
 * The fifth pass uses call data to determine the mappings in use.
 */
class LibraryAnalysis : public FlatCallAnalysis
{
public:
    // Equivalent to calling LibrarySummary(*calls()).
    LibraryAnalysis(InheritanceModel const& _model);

    // Describes all libraries in use by the bundle.
    std::shared_ptr<LibrarySummary const> libraries() const;

private:
    std::shared_ptr<LibrarySummary> m_libraries;
};

// -------------------------------------------------------------------------- //

/**
 * The sixth pass of analysis ensures that addresses are used appropriately,
 * and computes the required parameters for compositional reasoning.
 */
class FlatAddressAnalysis : public LibraryAnalysis
{
public:
    // Equivalent to calling FlatAddressAnalysis(_concrete, _clients,
    // model_cost()), followed by some error handling and analysis.
    // TODO(scottwe): deprecate _full.
    FlatAddressAnalysis(
        InheritanceModel const& _model,
        std::vector<SourceUnit const*> _full,
        size_t _clients,
        bool _concrete_clients
    );

    // Describes the address requirements of the bundle.
    std::shared_ptr<MapIndexSummary const> addresses() const;

private:
    std::shared_ptr<MapIndexSummary> m_addresses;
};

// -------------------------------------------------------------------------- //

/**
 * The final pass generates stand-alone modules such as the type translator and
 * the call state environment.
 */
class AnalysisStack : public FlatAddressAnalysis
{
public:
    // Initializes all analyses which can be performed after analying the
    // address access patterns. The _model parameter is used to specify the
    // contracts to encode. The _clients field gives the number of distinguished
    // clients, while _concrete_clients escalates all clients to a concrete
    // execution. The _escalates_reqs parameter will force all requirements to
    // be escalated into assertions.
    // TODO(scottwe): deprecate _full.
    AnalysisStack(
        InheritanceModel const& _model,
        std::vector<SourceUnit const*> _full,
        size_t _clients,
        bool _concrete_clients,
        bool _escalates_reqs
    );

    // Characterizes the environment needed by each call.
    std::shared_ptr<CallState const> environment() const;

    // Returns the type analyzer.
    std::shared_ptr<TypeAnalyzer const> types() const;

private:
    std::shared_ptr<CallState> m_environment;
    std::shared_ptr<TypeAnalyzer> m_types;
};

// -------------------------------------------------------------------------- //

}
}
}
