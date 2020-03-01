/**
 * @date 2019
 * This file operators utilities for traversing the Solidity AST. This is a
 * header-only library, consisting of simple, generative utilities.
 */

#include <libsolidity/modelcheck/utils/AST.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

VariableDeclaration const* member_access_to_decl(MemberAccess const& _access)
{
	auto const EXPR_TYPE = _access.expression().annotation().type;
    if (auto contract_type = dynamic_cast<ContractType const*>(EXPR_TYPE))
    {
        for (auto member : contract_type->contractDefinition().stateVariables())
        {
            if (member->name() == _access.memberName())
            {
                return member;
            }
        }
    }
    else if (auto struct_type = dynamic_cast<StructType const*>(EXPR_TYPE))
	{
        for (auto member : struct_type->structDefinition().members())
        {
            if (member->name() == _access.memberName())
            {
                return member.get();
            }
        }
	}
    return nullptr;
}

// -------------------------------------------------------------------------- //

}
}
}
