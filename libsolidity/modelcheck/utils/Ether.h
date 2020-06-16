/**
 * Data and helper functions for generating Ether-related methods.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/AST.h>

#include <map>
#include <utility>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utilities to generate Ether-related methods.
 */
class Ether
{
public:
    static std::string const TRANSFER;
    static std::string const SEND;
    static std::string const PAY;
};

// -------------------------------------------------------------------------- //

}
}
}
