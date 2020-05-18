/**
 * @date 2020
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 */

#include <libsolidity/modelcheck/analysis/ContractDependance.h>

#include <libsolidity/modelcheck/analysis/FunctionCall.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ContractDependance::ContractDependance(
    ContractDependance::DependancyAnalyzer const& _analyzer
) : m_contracts(_analyzer.m_contracts)
{
    // Does per-contract analysis.
    for (auto contract : m_contracts)
    {
        // Finds all interfaces of this contract.
        m_interfaces[contract] = _analyzer.get_interfaces_for(contract);

        // Does per-interface analysis.
        for (auto interface : m_interfaces[contract])
        {
            // Finds supercalls if not already populated.
            if (m_superchain.find(interface) == m_superchain.end())
            {
                m_superchain[interface]
                    = _analyzer.get_superchain_for(interface);
            }
        }
    }
}

// -------------------------------------------------------------------------- //

bool ContractDependance::is_deployed(ContractDefinition const* _actor) const
{
    return (m_contracts.find(_actor) != m_contracts.end());
}

// -------------------------------------------------------------------------- //

ContractDependance::SuperCalls const& ContractDependance::get_superchain(
    FunctionDefinition const* _func
) const
{
    auto res = m_superchain.find(_func);
    if (res != m_superchain.end())
    {
        return res->second;
    }
    throw std::runtime_error("Superchain requested on out-of-scope function.");
}

// -------------------------------------------------------------------------- //

ContractDependance::FuncInterface const& ContractDependance::get_interface(
    ContractDefinition const* _actor
) const
{
    auto res = m_interfaces.find(_actor);
    if (res != m_interfaces.end())
    {
        return res->second;
    }
    throw std::runtime_error("Interface requested on out-of-scope contract.");
}

// -------------------------------------------------------------------------- //

}
}
}
