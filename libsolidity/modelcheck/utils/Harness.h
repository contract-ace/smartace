/**
 * @date 2020
 * Data and helper functions for generating the harness. This is meant to reduce
 * code duplication.
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Details.h>

#include <cstdint>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utilities to genereate generic harness elements.
 */
class HarnessUtilities
{
public:
    // Appends a require statement to _block, conditioned on _cond.
    static void require(CBlockList & _block, CExprPtr _cond);

    // Produces a range between two values.
    static CExprPtr range(uint8_t _l, uint8_t _u, std::string const& _msg);

    // Generates a random byte of data, through a unique uninterpreted function.
    static CExprPtr byte(std::string const& _msg);
};

// -------------------------------------------------------------------------- //

}
}
}
