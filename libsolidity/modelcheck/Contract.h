/**
 * @date 2019
 * Data and helper functions for generating contracts. This is meant to reduce
 * code duplication.
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
 * Utilities to move a function from the Solidity representation to the c-model
 * representation.
 */
class ContractUtilities
{
public:
    static std::string address_member();
    static TypePointer address_type();

private:
    static AddressType ADDRESS_MEMBER_TYPE;
};

}
}
}
