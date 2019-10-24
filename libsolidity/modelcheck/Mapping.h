/**
 * @date 2019
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 */

#pragma once

#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * A collection of mapping-related utilities.
 */
class MappingUtilities
{
public:
    // The fields used by a map.
    static const std::string SET_FIELD;
    static const std::string CURR_FIELD;
    static const std::string DATA_FIELD;
    static const std::string ND_FIELD;
};

}
}
}
