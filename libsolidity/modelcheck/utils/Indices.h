/**
 * @date 2020
 * Data and helper functions for generating indices.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Tools for generating constants.
 */
class Indices
{
public:
    // Generates the name of a constant address global variable.
    static std::string const_global_name(dev::u256 _var);
};

}
}
}
