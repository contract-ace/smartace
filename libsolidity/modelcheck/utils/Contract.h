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
    // Returns the name of the address field of each contract.
    static std::string address_member();

    // Returns the name of the balance field of each contract.
    static std::string balance_member();

    // Returns the type of the address field of each contract.
    static TypePointer address_type();

    // Returns the type of the balance field of each contract.
    static TypePointer balance_type();

private:
    static AddressType ADDRESS_MEMBER_TYPE;
    static IntegerType BALANCE_MEMBER_TYPE;
};

}
}
}
