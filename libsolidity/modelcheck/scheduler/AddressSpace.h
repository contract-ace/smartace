/**
 * Decouples address logic from the scheduler. This allows for address models to
 * be interchanged or maintained without analysis of the entire scheduler.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Details.h>

#include <cstdint>
#include <memory>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class NondetSourceRegistry;
class PTGBuilder;

// -------------------------------------------------------------------------- //

/**
 * Describes the possible addresses, maintains their allocations, and provides a
 * utility to allocate distinct addresses.
 */
class AddressSpace
{
public:
    AddressSpace(
        std::shared_ptr<PTGBuilder const> _address_data,
        std::shared_ptr<NondetSourceRegistry> _nd_reg
    );

    // Generates statements in _block to map all constants to distinct values.
    void map_constants(CBlockList & _block) const;

private:
    // Stores the minimum allocatable address. This accounts for 0.
    const uint64_t MIN_ADDR = 1;

    // The maximum allocated address.
    const uint64_t MAX_ADDR;

    // Stores all parameters over the address space.
    std::shared_ptr<PTGBuilder const> m_address_data;

    std::shared_ptr<NondetSourceRegistry> m_nd_reg;

    // The last allocated address.
    uint64_t m_next_addr;
};

// -------------------------------------------------------------------------- //

}
}
}
