
#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/utils/Function.h>

#include <set>
#include <stdexcept>
#include <string>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

FlatContract::FlatContract(ContractDefinition const& _contract)
 : M_NAME(_contract.name()), m_raw(&_contract), m_fallback(nullptr)
{
    map<string, FunctionList> registered_functions;
    set<string> variable_names;
    for (auto c : _contract.annotation().linearizedBaseContracts)
    {
        // If this is an interface, there is nothing to do.
        if (c->isInterface()) continue;

        // Checks for functions with new signatures.
        for (auto f : c->definedFunctions())
        {
            // Searchs for a fallback.
            if (!m_fallback && f->isFallback())
            {
                m_fallback = f;
                continue;
            }

            // Accumulates constructors.
            if (f->isConstructor())
            {
                m_constructors.push_back(f);
                continue;
            }

            // Checks for duplicates.
            auto & entries = registered_functions[f->name()];
            bool found_match = false;
            for (auto candidate : entries)
            {
                found_match = collid(*f, *candidate);
                if (found_match) break;
            }
            if (found_match) continue;

            // It is new, so register it.
            entries.push_back(f);
            if (f->functionType(false))
            {
                m_public.push_back(f);
            }
            else
            {
                m_private.push_back(f);
            }
        }

        // Checks for new variables.
        for (auto v : c->stateVariables())
        {
            if (variable_names.insert(v->name()).second)
            {
                m_vars.push_back(v);
            }
        }
    }
}

FlatContract::FunctionList const& FlatContract::interface() const
{
    return m_public;
}

FlatContract::VariableList const& FlatContract::state_variables() const
{
    return m_vars;
}

string const& FlatContract::name() const
{
    return M_NAME;
}

FlatContract::FunctionList FlatContract::constructors() const
{
    return m_constructors;
}

FunctionDefinition const* FlatContract::fallback() const
{
    return m_fallback;
}

FunctionDefinition const&
    FlatContract::resolve(FunctionDefinition const& _func) const
{
    if (_func.functionType(false))
    {
        for (auto method : m_public)
        {
            if (collid(_func, *method)) return (*method);
        }
    }
    else
    {
        for (auto method : m_private)
        {
            if (collid(_func, *method)) return (*method);
        }
    }
    throw runtime_error("Could not resolve function against flat contract.");
}

ContractDefinition const* FlatContract::raw() const
{
    // TODO: deprecate.
    return m_raw;
}

// -------------------------------------------------------------------------- //

FlatModel::FlatModel(
    FlatModel::ContractList const _model, AllocationGraph const& _alloc_graph
)
{
    // Iterates through all children.
    set<ContractDefinition const*> visited;
    FlatModel::ContractList pending = _model;
    for (size_t i = 0; i < pending.size(); ++i)
    {
        // Checks if this is a new contract.
        auto contract = pending[i];
        if (!visited.insert(contract).second) continue;

        // Records the contract.
        m_contracts.push_back(make_shared<FlatContract>(*contract));
        m_lookup[contract] = m_contracts.back();

        // Adds children to the list.
        for (auto child : _alloc_graph.children_of(contract))
        {
            pending.push_back(child.type);
        }
    }

    // Performs a second pass to add parents and children.
    auto processed = visited;
    for (auto contract : processed)
    {
        for (auto parent : contract->annotation().linearizedBaseContracts)
        {
            if (!visited.insert(parent).second) continue;
            m_lookup[parent] = make_shared<FlatContract>(*parent);
        }

        for (auto child : _alloc_graph.children_of(contract))
        {
            ChildRecord record{m_lookup[child.type], child.dest->name()};
            m_children[m_lookup[contract].get()].push_back(std::move(record));
        }
    }

    // Records the deployed contracts in the model.
    for (auto contract : _model)
    {
        m_bundle.push_back(m_lookup[contract]);
    }
}

FlatModel::FlatList FlatModel::bundle() const { return m_bundle; }

FlatModel::FlatList FlatModel::view() const { return m_contracts; }

shared_ptr<FlatContract> FlatModel::get(ContractDefinition const& _src) const
{
    auto match = m_lookup.find(&_src);
    if (match != m_lookup.end())
    {
        return match->second;
    }
    return nullptr;
}

std::vector<FlatModel::ChildRecord>
    FlatModel::children_of(FlatContract const& _contract) const
{
    auto match = m_children.find(&_contract);
    if (match != m_children.end())
    {
        return match->second;
    }
    return {};
}

// -------------------------------------------------------------------------- //

}
}
}
