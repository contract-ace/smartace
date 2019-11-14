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

string ContractUtilities::address_member()
{
    return VariableScopeResolver::rewrite("address", true, VarContext::STRUCT);
}

TypePointer ContractUtilities::address_type()
{
    return &ContractUtilities::ADDRESS_MEMBER_TYPE;
}

}
}
}
