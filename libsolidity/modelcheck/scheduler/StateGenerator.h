/**
 * Utility to generate the next global state, from within the scheduler.
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

class CallState;
class MapIndexSummary;
class TypeAnalyzer;

// -------------------------------------------------------------------------- //

/**
 * Utility used to setup, and maintain, EVM global state.
 */
class StateGenerator
{
public:
    StateGenerator(
        CallState const& _statedata,
        TypeAnalyzer const& _converter,
        MapIndexSummary const& _addrdata,
        bool _use_lockstep_time
    );

    // Declares all state variables used to maintain EVM state.
    void declare(CBlockList & _block) const;

    // Generate the instructions required to update the call state.
    void update(CBlockList & _block) const;

    // Generates a value for a payable method.
    void pay(CBlockList & _block) const;

private:
    CallState const& M_STATEDATA;
	TypeAnalyzer const& M_CONVERTER;
    MapIndexSummary const& M_MAPDATA;

    // When true, time and blocknumber advance in lockstep.
    bool const M_USE_LOCKSTEP_TIME;

    // State variable used to signal when time should proceed (in lockstep).
    std::shared_ptr<CVarDecl> const M_STEPVAR;
};

// -------------------------------------------------------------------------- //

}
}
}
