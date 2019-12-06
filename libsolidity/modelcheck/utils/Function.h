/**
 * @date 2019
 * Data and helper functions for generating functions. This is meant to reduce
 * code duplication.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/codegen/Core.h>
#include <memory>
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
class FunctionUtilities
{
public:
    // It is assumed that _e is a raw value, meant to be interpreted as type _t.
    // If _t is a wrapper, then the wrap initialization function must be invoked
    // on _e. Otherwise, _e is passed reflectively. This function implements
    // such logic.
    static CExprPtr try_to_wrap(Type const& _type, CExprPtr _expr);

    // Computers the name of a function from its name, implementing class, and
    // the class it will be used by. The class it will be used by differs from
    // the base class when the function is overloaded.
    static std::string name(
        FunctionDefinition const& _def,
        ContractDefinition const& _src,
        ContractDefinition const& _for
    );

    // Produces the name for the i-th modifier, from a base name. This interface
    // is internal to the function calling it, so it need not be globally
    // accessible (ie. resolvable in TypeConverter by ASTNode address).
    static std::string modifier_name(std::string _base, size_t _i);
    static std::string base_name(std::string _base);

    // Produces the name for a constructor, from a base contract and derived
    // contract. This interface is internal to the function calling it, so it
    // not be globally accessible (ie. resolvable in the TypeConverter by
    // ASTNode address).
    static std::string ctor_name(
        ContractDefinition const& _src, ContractDefinition const& _for
    );
};

}
}
}
