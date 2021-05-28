#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Library.h>
#include <libsolidity/modelcheck/analysis/Structure.h>
#include <libsolidity/modelcheck/analysis/TightBundle.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

BaseAnalysis::BaseAnalysis()
 : m_structure_store(make_shared<StructureStore>()) {}

// -------------------------------------------------------------------------- //

AllocationAnalysis::AllocationAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
)
{
	(void) _settings;

    m_allocation_graph = make_shared<AllocationGraph>(_model);

	auto const& VIOLATIONS = m_allocation_graph->violations();
	if (!VIOLATIONS.empty())
	{
		// TODO: better error.
		auto const& count = to_string(VIOLATIONS.size());
        throw runtime_error("AllocationAnalysis violations: " + count);
    }
}

shared_ptr<AllocationGraph const> AllocationAnalysis::allocations() const
{
    return m_allocation_graph;
}

// -------------------------------------------------------------------------- //

InheritanceAnalysis::InheritanceAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
): AllocationAnalysis(_model, _settings)
{
	m_flat_model
		= make_shared<FlatModel>(_model, *allocations(), *m_structure_store);
}

shared_ptr<FlatModel const> InheritanceAnalysis::model() const
{
    return m_flat_model;
}

// -------------------------------------------------------------------------- //

ContractExprAnalysis::ContractExprAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
): InheritanceAnalysis(_model, _settings)
{
	m_contracts
		= make_shared<ContractExpressionAnalyzer>(model(), allocations());
}

shared_ptr<ContractExpressionAnalyzer const>
	ContractExprAnalysis::contracts() const
{
    return m_contracts;
}

// -------------------------------------------------------------------------- //

FlatCallAnalysis::FlatCallAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
): ContractExprAnalysis(_model, _settings)
{
	m_call_graph = make_shared<CallGraph>(contracts(), model());
}

shared_ptr<CallGraph const> FlatCallAnalysis::calls() const
{
    return m_call_graph;
}

// -------------------------------------------------------------------------- //

LibraryAnalysis::LibraryAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
): FlatCallAnalysis(_model, _settings)
{
	m_libraries = make_shared<LibrarySummary>(*calls(), *m_structure_store);
}

shared_ptr<LibrarySummary const> LibraryAnalysis::libraries() const
{
	return m_libraries;
}

// -------------------------------------------------------------------------- //

EnvironmentAnalysis::EnvironmentAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
): LibraryAnalysis(_model, _settings)
{
	m_environment = make_shared<CallState>(*calls(), _settings.escalate_reqs);
}

shared_ptr<CallState const> EnvironmentAnalysis::environment() const
{
	return m_environment;
}

// -------------------------------------------------------------------------- //

TightBundleAnalysis::TightBundleAnalysis(
	InheritanceModel const& _model, AnalysisSettings const&_settings
): EnvironmentAnalysis(_model, _settings)
{
	m_tight_bundle = make_shared<TightBundleModel>(
		*model(), *environment(), _settings.allow_fallbacks
	);
}

shared_ptr<TightBundleModel const> TightBundleAnalysis::tight_bundle() const
{
	return m_tight_bundle;
}

// -------------------------------------------------------------------------- //

FlatAddressAnalysis::FlatAddressAnalysis(
	InheritanceModel const& _model,
	std::vector<SourceUnit const*> _full,
	AnalysisSettings const&_settings
): TightBundleAnalysis(_model, _settings)
{
	m_addresses = make_shared<MapIndexSummary>(
		_settings.use_concrete_users,
		_settings.persistent_user_count,
		tight_bundle()->size()
	);

	for (auto const* ast : _full)
	{
		auto c = ASTNode::filteredNodes<ContractDefinition>(ast->nodes());
		for (auto contract : c)
		{
			m_addresses->extract_literals(*contract);
		}
	}
	for (auto const* ast: _full)
	{
		auto c = ASTNode::filteredNodes<ContractDefinition>(ast->nodes());
		for (auto contract : c)
		{
			m_addresses->compute_interference(*contract);
		}
	}

	auto const& VIOLATIONS = m_addresses->violations();
	if (!VIOLATIONS.empty())
	{
		// TODO: better error.
		auto const& count = std::to_string(VIOLATIONS.size());
		throw runtime_error("FlatAddressAnalysis violations: " + count);
	}
}

shared_ptr<MapIndexSummary const> FlatAddressAnalysis::addresses() const
{
	return m_addresses;
}

// -------------------------------------------------------------------------- //

AnalysisStack::AnalysisStack(
	InheritanceModel const& _model,
	std::vector<SourceUnit const*> _full,
	AnalysisSettings const&_settings
): FlatAddressAnalysis(_model, _full, _settings)
{
	m_types = make_shared<TypeAnalyzer>();

	// TODO: deprecate.
	for (auto const* ast : _full)
	{
		m_types->record(*ast);
	}
}

shared_ptr<TypeAnalyzer const> AnalysisStack::types() const
{
	return m_types;
}


// -------------------------------------------------------------------------- //

}
}
}
