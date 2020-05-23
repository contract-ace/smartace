/**
 * @date 2019
 * Data and helper functions for generating contracts. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/utils/Contract.h>

#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

AddressType const ContractUtilities::ADDRESS_MEMBER_TYPE(
    StateMutability::Payable
);

IntegerType const ContractUtilities::BALANCE_MEMBER_TYPE(256);

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

FunctionDefinition const& ContractUtilities::fallback(
    ContractDefinition const& _c
)
{
    for (auto contract : _c.annotation().linearizedBaseContracts)
    {
        if (contract->fallbackFunction())
        {
            return (*contract->fallbackFunction());
        }
    }
    throw runtime_error("Fallback extracted from contract without fallback.");
}

// -------------------------------------------------------------------------- //

}
}
}
