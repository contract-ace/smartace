
#include <libsolidity/modelcheck/analysis/Inheritance.h>

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
            if (!(f->isImplemented() && f->isPublic())) continue;

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
            m_methods.push_back(f);
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
    return m_methods;
}

FlatContract::VariableList const& FlatContract::state_variables() const
{
    return m_vars;
}

// -------------------------------------------------------------------------- //

}
}
}
