#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Library.h>
#include <libsolidity/modelcheck/analysis/StringLookup.h>
#include <libsolidity/modelcheck/analysis/Structure.h>
#include <libsolidity/modelcheck/analysis/TightBundle.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

namespace
{
static void check_allocation_graph_errs(shared_ptr<AllocationGraph> _graph)
{
	auto const& VIOLATIONS = _graph->violations();
	if (!VIOLATIONS.empty())
	{
		// TODO: better error.
		auto const& count = to_string(VIOLATIONS.size());
        throw runtime_error("AllocationAnalysis violations: " + count);
    }
}

static void check_address_errs(shared_ptr<PTGBuilder> _summary)
{
	auto const& VIOLATIONS = _summary->violations();
	if (!VIOLATIONS.empty())
	{
		// TODO: better error.
		auto const& count = std::to_string(VIOLATIONS.size());
		throw runtime_error("FlatAddressAnalysis violations: " + count);
	}
}
}

// -------------------------------------------------------------------------- //

AnalysisStack::AnalysisStack(
	InheritanceModel const& _model,
	std::vector<SourceUnit const*> _full,
	AnalysisSettings const&_settings
)
{
	m_structure_store = make_shared<StructureStore>();

    m_allocation_graph = make_shared<AllocationGraph>(_model);
	check_allocation_graph_errs(m_allocation_graph);

	m_flat_model = make_shared<FlatModel>(
		_model, *m_allocation_graph, *m_structure_store
	);

	m_contracts = make_shared<ContractExpressionAnalyzer>(
		m_flat_model, m_allocation_graph
	);

	m_call_graph = make_shared<CallGraph>(m_contracts, m_flat_model);

	m_libraries = make_shared<LibrarySummary>(
		*m_call_graph, *m_structure_store
	);

	m_environment = make_shared<CallState>(
		*m_call_graph, _settings.escalate_reqs
	);

	m_tight_bundle = make_shared<TightBundleModel>(
		*m_flat_model, *m_environment, _settings.allow_fallbacks
	);

	// TODO: deprecate the use of _full.
	m_types = make_shared<TypeAnalyzer>(_full, *m_call_graph);

	m_addresses = make_shared<PTGBuilder>(
		m_types->map_db(),
		*m_flat_model,
		*m_call_graph,
		_settings.use_concrete_users,
		m_tight_bundle->size(),
		_settings.aux_user_count
	);
	check_address_errs(m_addresses);

	m_strings = make_shared<StringLookup>(*m_flat_model, *m_call_graph);
}

shared_ptr<StructureStore const> AnalysisStack::structures() const
{
	return m_structure_store;
}

shared_ptr<AllocationGraph const> AnalysisStack::allocations() const
{
    return m_allocation_graph;
}

shared_ptr<FlatModel const> AnalysisStack::model() const
{
    return m_flat_model;
}

shared_ptr<ContractExpressionAnalyzer const> AnalysisStack::contracts() const
{
    return m_contracts;
}

shared_ptr<CallGraph const> AnalysisStack::calls() const
{
    return m_call_graph;
}

shared_ptr<LibrarySummary const> AnalysisStack::libraries() const
{
	return m_libraries;
}

shared_ptr<CallState const> AnalysisStack::environment() const
{
	return m_environment;
}

shared_ptr<TightBundleModel const> AnalysisStack::tight_bundle() const
{
	return m_tight_bundle;
}

shared_ptr<PTGBuilder const> AnalysisStack::addresses() const
{
	return m_addresses;
}

shared_ptr<StringLookup const> AnalysisStack::strings() const
{
	return m_strings;
}

shared_ptr<TypeAnalyzer const> AnalysisStack::types() const
{
	return m_types;
}

// -------------------------------------------------------------------------- //

}
}
}
