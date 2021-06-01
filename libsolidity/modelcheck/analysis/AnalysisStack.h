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
class PTGBuilder;
class StringLookup;
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
    size_t aux_user_count = 0;
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
 * Initializes analysis passes, and resolves all dependencies between passes.
 */
class AnalysisStack
{
public:
    // The _model parameter is used to specify the contracts to encode. All
    // other arguments are specified by _settings.
    // TODO(scottwe): deprecate _full.
    AnalysisStack(
        InheritanceModel const& _model,
        std::vector<SourceUnit const*> _full,
        AnalysisSettings const& _settings
    );

    // Describes all structures that are used by one or more contracts.
    std::shared_ptr<StructureStore const> structures() const;

    // Describes all virtual allocations in the bundle.
    std::shared_ptr<AllocationGraph const> allocations() const;

    // Describes all contracts in the model.
    std::shared_ptr<FlatModel const> model() const;

    // Describes all calls made within the bundle.
    std::shared_ptr<ContractExpressionAnalyzer const> contracts() const;

    // Describes all calls made within the bundle.
    std::shared_ptr<CallGraph const> calls() const;

    // Describes all libraries in use by the bundle.
    std::shared_ptr<LibrarySummary const> libraries() const;

    // Characterizes the environment needed by each call.
    std::shared_ptr<CallState const> environment() const;

    // A unique identity for each contract instance.
    std::shared_ptr<TightBundleModel const> tight_bundle() const;

    // Describes the address requirements of the bundle.
    std::shared_ptr<PTGBuilder const> addresses() const;

    // Returns a mapping from string literals to strings.
    std::shared_ptr<StringLookup const> strings() const;

    // Returns the type analyzer.
    std::shared_ptr<TypeAnalyzer const> types() const;

private:
    std::shared_ptr<StructureStore> m_structure_store;
    std::shared_ptr<AllocationGraph> m_allocation_graph;
    std::shared_ptr<FlatModel> m_flat_model;
    std::shared_ptr<ContractExpressionAnalyzer> m_contracts;
    std::shared_ptr<CallGraph> m_call_graph;
    std::shared_ptr<LibrarySummary> m_libraries;
    std::shared_ptr<CallState> m_environment;
    std::shared_ptr<TightBundleModel> m_tight_bundle;
    std::shared_ptr<PTGBuilder> m_addresses;
    std::shared_ptr<StringLookup> m_strings;
    std::shared_ptr<TypeAnalyzer> m_types;
};

// -------------------------------------------------------------------------- //

}
}
}
