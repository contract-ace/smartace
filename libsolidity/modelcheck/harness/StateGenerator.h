/**
 * Utility to generate the next global state, from within the harness.
 * @date 2020
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Details.h>

#include <list>
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
class TypeConverter;

// -------------------------------------------------------------------------- //

/**
 * Utility used to setup, and maintain, EVM global state.
 */
class StateGenerator
{
public:
    StateGenerator(
        CallState const& _statedata,
        TypeConverter const& _converter,
        MapIndexSummary const& _addrdata,
        bool _use_lockstep_time
    );

    // Declares all state variables used to maintain EVM state.
    void declare(CBlockList & _block) const;

    // Generate the instructions required to update the call state.
    void update(
        CBlockList & _block,
        std::list<std::shared_ptr<CMemberAccess>> const& _addrvars
    ) const;

    // Generates a value for a payable method.
    void pay(CBlockList & _block) const;

private:
    // When true, time and blocknumber advance in lockstep.
    const bool M_USE_LOCKSTEP_TIME;

    // State variable used to signal when time should proceed (in lockstep).
    std::shared_ptr<CVarDecl> M_STEPVAR;

    CallState const& m_statedata;
	TypeConverter const& m_converter;
    MapIndexSummary const& m_addrdata;
};

// -------------------------------------------------------------------------- //

}
}
}
