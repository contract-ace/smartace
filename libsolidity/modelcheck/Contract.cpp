/**
 * @date 2019
 * Data and helper functions for generating contracts. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/Contract.h>

#include <libsolidity/modelcheck/VariableScopeResolver.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

AddressType ContractUtilities::ADDRESS_MEMBER_TYPE(StateMutability::Payable);

IntegerType ContractUtilities::BALANCE_MEMBER_TYPE(256);

string ContractUtilities::address_member()
{
    return VariableScopeResolver::rewrite("address", true, VarContext::STRUCT);
}

string ContractUtilities::balance_member()
{
    return VariableScopeResolver::rewrite("balance", true, VarContext::STRUCT);
}

TypePointer ContractUtilities::address_type()
{
    return &ContractUtilities::ADDRESS_MEMBER_TYPE;
}

TypePointer ContractUtilities::balance_type()
{
    return &ContractUtilities::BALANCE_MEMBER_TYPE;
}

}
}
}
