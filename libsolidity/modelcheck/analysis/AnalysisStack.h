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
class StructureStore;
class TightBundleModel;
class TypeAnalyzer;

using InheritanceModel = std::vector<ContractDefinition const*>;

// -------------------------------------------------------------------------- //

/**
 * All settings for the analysis stack.
 */
struct AnalysisSettings
{
    // The number of additional, persistent users, required by the properties.
    size_t persistent_user_count = 0;
    // If true, each user is treated as if it were in a small model (rather than
    // an equivalence class).
    bool use_concrete_users = false;
    // If true, contracts may be represented by global variables (e.g., if the
    // client's fallback an be executed through a transfer or send).
    bool use_global_contracts = false;
    // If true, all require statements will be replaced by assertion statements.
    bool escalate_reqs = false;
    // If true, fallbacks through sends and transfers are allowed.
    bool allow_fallbacks = false;
};

// -------------------------------------------------------------------------- //

/**
 * Sets up underlying structures needed by the analysis stack.
 */
class BaseAnalysis
{
public:
    BaseAnalysis();

protected:
    std::shared_ptr<StructureStore> m_structure_store;
};

// -------------------------------------------------------------------------- //

/**
 * This pass of analysis devirtualizes contracts.
 */
class AllocationAnalysis : public BaseAnalysis
{
public:
    // Equivalent to AllocationGraph(_model), with some additional error
    // handling and post analysis summary.
    AllocationAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // Describes all virtual allocations in the bundle.
    std::shared_ptr<AllocationGraph const> allocations() const;

private:
    std::shared_ptr<AllocationGraph> m_allocation_graph;
};

// -------------------------------------------------------------------------- //

/**
 * This pass of analysis uses the devirtualization graph to build a model free
 * from hierarchies.
 */
class InheritanceAnalysis : public AllocationAnalysis
{
public:
    // Equivalent to calling FlatModel(_model, *allocations()).
    InheritanceAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // Describes all contracts in the model.
    std::shared_ptr<FlatModel const> model() const;

private:
    std::shared_ptr<FlatModel> m_flat_model;
};

// -------------------------------------------------------------------------- //

/**
 * This pass of analysis upcasts all contract return values. The end result is
 * a mapping from contract expressions to upcast contracts.
 */
class ContractExprAnalysis : public InheritanceAnalysis
{
public:
    // Equivalent to ContractExpressionAnalyzer(*model(), *allocations()).
    ContractExprAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // Describes all calls made within the bundle.
    std::shared_ptr<ContractExpressionAnalyzer const> contracts() const;

private:
    std::shared_ptr<ContractExpressionAnalyzer> m_contracts;
};

// -------------------------------------------------------------------------- //

/**
 * This pass of analysis constructs a call graph for all contracts in the model.
 */
class FlatCallAnalysis : public ContractExprAnalysis
{
public:
    // Equivalent to calling CallGraph(*model(), *allocations()).
    FlatCallAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // Describes all calls made within the bundle.
    std::shared_ptr<CallGraph const> calls() const;

private:
    std::shared_ptr<CallGraph> m_call_graph;
};

// -------------------------------------------------------------------------- //

/**
 * This pass uses call data to determine the mappings in use.
 */
class LibraryAnalysis : public FlatCallAnalysis
{
public:
    // Equivalent to calling LibrarySummary(*calls()).
    LibraryAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // Describes all libraries in use by the bundle.
    std::shared_ptr<LibrarySummary const> libraries() const;

private:
    std::shared_ptr<LibrarySummary> m_libraries;
};

// -------------------------------------------------------------------------- //

/**
 * This pass uses call data to determine the minimal blockchain environment.
 */
class EnvironmentAnalysis : public LibraryAnalysis
{
public:
    // Equivalent to calling CallState(*calls()).
    EnvironmentAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // Characterizes the environment needed by each call.
    std::shared_ptr<CallState const> environment() const;

private:
    std::shared_ptr<CallState> m_environment;
};

// -------------------------------------------------------------------------- //

/**
 * This pass of analysis assigns unique identifiers to each contract instance.
 * This is different from FlatContracts. There may be two or more instances of a
 * single flat contract in a tightly coupled smart contract bundle.
 */
class TightBundleAnalysis : public EnvironmentAnalysis
{
public:
    // Equivalent to TightBundleModel(*model()).
    TightBundleAnalysis(
        InheritanceModel const& _model, AnalysisSettings const& _settings
    );

    // A unique identity for each contract instance.
    std::shared_ptr<TightBundleModel const> tight_bundle() const;

private:
    std::shared_ptr<TightBundleModel> m_tight_bundle;
};

// -------------------------------------------------------------------------- //

/**
 * Thiss pass of analysis ensures that addresses are used appropriately, and
 * computes the required parameters for compositional reasoning.
 */
class FlatAddressAnalysis : public TightBundleAnalysis
{
public:
    // Equivalent to calling FlatAddressAnalysis(_concrete, _clients,
    // tight_bundle()->size()), followed by some error handling and analysis.
    // TODO(scottwe): deprecate _full.
    FlatAddressAnalysis(
        InheritanceModel const& _model,
        std::vector<SourceUnit const*> _full,
        AnalysisSettings const& _settings
    );

    // Describes the address requirements of the bundle.
    std::shared_ptr<MapIndexSummary const> addresses() const;

private:
    std::shared_ptr<MapIndexSummary> m_addresses;
};

// -------------------------------------------------------------------------- //

/**
 * This pass generates stand-alone modules such as the type translator and the
 * call state environment.
 */
class AnalysisStack : public FlatAddressAnalysis
{
public:
    // Initializes all analyses which can be performed after analying the
    // address access patterns. The _model parameter is used to specify the
    // contracts to encode. All other arguments are specified by _settings.
    // TODO(scottwe): deprecate _full.
    AnalysisStack(
        InheritanceModel const& _model,
        std::vector<SourceUnit const*> _full,
        AnalysisSettings const& _settings
    );

    // Returns the type analyzer.
    std::shared_ptr<TypeAnalyzer const> types() const;

private:
    std::shared_ptr<TypeAnalyzer> m_types;
};

// -------------------------------------------------------------------------- //

}
}
}
