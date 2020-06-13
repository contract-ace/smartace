#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
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

	m_model_cost = 0;
	for (auto actor : _model)
	{
		m_model_cost += m_allocation_graph->cost_of(actor);
	}
}

shared_ptr<AllocationGraph const> AllocationAnalysis::allocations() const
{
    return m_allocation_graph;
}

size_t AllocationAnalysis::model_cost() const { return m_model_cost; }

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

FlatCallAnalysis::FlatCallAnalysis(InheritanceModel const& _model)
 : InheritanceAnalysis(_model)
{
	m_call_graph = make_shared<CallGraph>(allocations(), model());
}

shared_ptr<CallGraph const> FlatCallAnalysis::calls() const
{
    return m_call_graph;
}

// -------------------------------------------------------------------------- //

FlatAddressAnalysis::FlatAddressAnalysis(
	InheritanceModel const& _model,
	std::vector<SourceUnit const*> _full,
	size_t _clients,
	bool _concrete_clients
): FlatCallAnalysis(_model)
{
	m_addresses = make_shared<MapIndexSummary>(
		_concrete_clients, _clients, model_cost()
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
	bool _concrete_clients
): FlatAddressAnalysis(_model, _full, _clients, _concrete_clients)
{
	m_environment = make_shared<CallState>(*calls());
	m_types = make_shared<TypeAnalyzer>(addresses()->size());

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
