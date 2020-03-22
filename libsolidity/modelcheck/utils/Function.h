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

// -------------------------------------------------------------------------- //

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

    // Produces the name for the i-th modifier, from a base name. This interface
    // is internal to the function calling it, so it need not be globally
    // accessible (ie. resolvable in TypeConverter by ASTNode address).
    static std::string modifier_name(std::string _base, size_t _i);
    static std::string base_name(std::string _base);

    // Returns the name for the destination used in contract initialization.
    static std::string init_var();
};

// -------------------------------------------------------------------------- //

/**
 * This class manages the specialization of a FunctionDefinition in Solidity.
 * Solidity allows functions to be extended and inherited. In general, for some
 * class hierarchy ... -> A -> ... -> B -> ... -> C, it is possible that C
 * inherits a function foo from B which extends a foo in A. Therefore, to
 * specialize foo to C, the transpiler must generate instances of A.foo() and
 * B.foo(), both specialized for contract C. This structure encodes the notion
 * of "Function foo, as used by contract A, intended for contract C".
 * 
 * Utilities are implemented to simply processes, such as name generation.
 */
class FunctionSpecialization
{
public:
    FunctionSpecialization(FunctionDefinition const& _def);

    FunctionSpecialization(
        FunctionDefinition const& _def, ContractDefinition const& _for
    );

    FunctionSpecialization(
        FunctionDefinition const& _def,
        ContractDefinition const& _src,
        ContractDefinition const& _for
    );

    std::unique_ptr<FunctionSpecialization> super() const;

    std::string name() const;

    ContractDefinition const& source() const;

    ContractDefinition const& useby() const;

    FunctionDefinition const& func() const;

private:
    static ContractDefinition const& get_scope(FunctionDefinition const& _func);

    FunctionDefinition const* M_CALL;
    ContractDefinition const* M_SRC;
    ContractDefinition const* M_USER;
};

// -------------------------------------------------------------------------- //

}
}
}
