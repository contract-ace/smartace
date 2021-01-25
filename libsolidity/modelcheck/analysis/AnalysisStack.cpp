#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Library.h>
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

AllocationAnalysis::AllocationAnalysis(InheritanceModel const& _model)
{
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

InheritanceAnalysis::InheritanceAnalysis(InheritanceModel const& _model)
 : AllocationAnalysis(_model)
{
	m_flat_model = make_shared<FlatModel>(_model, *allocations());
}

shared_ptr<FlatModel const> InheritanceAnalysis::model() const
{
    return m_flat_model;
}

// -------------------------------------------------------------------------- //

TightBundleAnalysis::TightBundleAnalysis(InheritanceModel const& _model)
 : InheritanceAnalysis(_model)
{
	m_tight_bundle = make_shared<TightBundleModel>(*model());
}

shared_ptr<TightBundleModel const> TightBundleAnalysis::tight_bundle() const
{
	return m_tight_bundle;
}

// -------------------------------------------------------------------------- //

ContractExprAnalysis::ContractExprAnalysis(InheritanceModel const& _model)
 : TightBundleAnalysis(_model)
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

FlatCallAnalysis::FlatCallAnalysis(InheritanceModel const& _model)
 : ContractExprAnalysis(_model)
{
	m_call_graph = make_shared<CallGraph>(contracts(), model());
}

shared_ptr<CallGraph const> FlatCallAnalysis::calls() const
{
    return m_call_graph;
}

// -------------------------------------------------------------------------- //

LibraryAnalysis::LibraryAnalysis(InheritanceModel const& _model)
 : FlatCallAnalysis(_model)
{
	m_libraries = make_shared<LibrarySummary>(*calls());
}

shared_ptr<LibrarySummary const> LibraryAnalysis::libraries() const
{
	return m_libraries;
}

// -------------------------------------------------------------------------- //

FlatAddressAnalysis::FlatAddressAnalysis(
	InheritanceModel const& _model,
	std::vector<SourceUnit const*> _full,
	size_t _clients,
	bool _concrete_clients
): LibraryAnalysis(_model)
{
	m_addresses = make_shared<MapIndexSummary>(
		_concrete_clients, _clients, tight_bundle()->size()
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
	size_t _clients,
	bool _concrete_clients,
	bool _escalates_reqs
): FlatAddressAnalysis(_model, _full, _clients, _concrete_clients)
{
	m_environment = make_shared<CallState>(*calls(), _escalates_reqs);
	m_types = make_shared<TypeAnalyzer>();

	// TODO: deprecate.
	for (auto const* ast : _full)
	{
		m_types->record(*ast);
	}
}

shared_ptr<CallState const> AnalysisStack::environment() const
{
	return m_environment;
}

shared_ptr<TypeAnalyzer const> AnalysisStack::types() const
{
	return m_types;
}


// -------------------------------------------------------------------------- //

}
}
}
