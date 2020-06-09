/**
 * Utilities for interacting with the abstract address domain.
 * 
 * @date 2020
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

// -------------------------------------------------------------------------- //

/**
 * Tools for generating constants.
 */
class AbstractAddressDomain
{
public:
    // Generates the name of a constant address global variable.
    static std::string literal_name(dev::u256 _var);
};

// -------------------------------------------------------------------------- //

}
}
}
