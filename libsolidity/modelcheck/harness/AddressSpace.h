/**
 * Decouples address logic from the harness. This allows for address models to
 * be interchanged or maintained without analysis of the entire harness.
 * @date 2020
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Details.h>

#include <cstdint>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class MapIndexSummary;

// -------------------------------------------------------------------------- //

/**
 * Describes the possible addresses, maintains their allocations, and provides a
 * utility to allocate distinct addresses.
 */
class AddressSpace
{
public:
    AddressSpace(MapIndexSummary const& _addrdata);

    // Returns a unique address. If all possible addresses have been expended,
    // an exception is raised.
    uint64_t reserve();

    // Generates statements in _block to map all constants to distinct values.
    void map_constants(CBlockList & _block) const;

private:
    // Stores the minimum allocatable address.
    const uint64_t MIN_ADDR;

    // The maximum allocated address.
    const uint64_t MAX_ADDR;

    // The last allocated address.
    uint64_t m_next_addr;

    // Stores all parameters over the address space.
    MapIndexSummary const& m_addrdata;
};

// -------------------------------------------------------------------------- //

}
}
}
