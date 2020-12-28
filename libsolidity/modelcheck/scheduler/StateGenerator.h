/**
 * Utility to generate the next global state, from within the scheduler.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Details.h>

#include <memory>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AnalysisStack;
class MapIndexSummary;
class NondetSourceRegistry;

// -------------------------------------------------------------------------- //

/**
 * Utility used to setup, and maintain, EVM global state.
 */
class StateGenerator
{
public:
    StateGenerator(
        std::shared_ptr<AnalysisStack const> _stack,
        std::shared_ptr<NondetSourceRegistry> _nd_reg,
        bool _use_lockstep_time
    );

    // Declares all state variables used to maintain EVM state.
    void declare(CBlockList & _block) const;

    // Generate the instructions required to update the call state.
    void update_global(CBlockList & _block) const;

    //
    void update_local(CBlockList & _block) const;

    // Generates a value for a payable method.
    void pay(CBlockList & _block) const;

private:
    // When true, time and blocknumber advance in lockstep.
    bool const M_USE_LOCKSTEP_TIME;

    // Utility variable to track the address of the last sender.
    static const std::string LAST_SENDER;

    std::shared_ptr<AnalysisStack const> m_stack;

    std::shared_ptr<NondetSourceRegistry> m_nd_reg;
};

// -------------------------------------------------------------------------- //

}
}
}
