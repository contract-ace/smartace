/**
 * Provides the code required to flatten inheritance hierarchies.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <list>
#include <map>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Creates a flat interface around the contract.
 */
class FlatContract
{
public:
    using FunctionList = std::list<FunctionDefinition const*>;
    using VariableList = std::list<VariableDeclaration const*>;

    // Flattens the inheritance tree of _contract.
    FlatContract(ContractDefinition const& _contract);

    // Returns all exposed methods of the contract.
    FunctionList const& interface() const;

    // Returns all inherited variables of the contract.
    VariableList const& state_variables() const;

private:
    FunctionList m_methods;
    VariableList m_vars;
};

// -------------------------------------------------------------------------- //

}
}
}
