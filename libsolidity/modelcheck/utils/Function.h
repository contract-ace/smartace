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

    // Returns the name for the destination used in contract initialization.
    static std::string init_var();
};

// -------------------------------------------------------------------------- //

/**
 * This class manages the specialization of a FunctionDefinition in Solidity.
 * Solidity allows functions to be extended and inherited. In general, for some
 * class hierarchy ... -> A -> ... -> B -> ... -> C, it is possible that C
 * inherits a function foo from B which extends a method foo in A. Therefore, to
 * specialize foo to C, SmartACE must generate instances of A.foo() and B.foo(),
 * both specialized for contract C. This structure encodes the notion of
 * "Function foo, as used by contract A, intended for contract C".
 * 
 * Utilities are implemented to simply processes, such as name generation.
 */
class FunctionSpecialization
{
public:
    // Builds the top-level specialization for _def. That is a call to _def, as
    // generated for use by _def against the contract _def was defined for.
    FunctionSpecialization(FunctionDefinition const& _def);

    // Specializes _def for use in child contract _for.
    FunctionSpecialization(
        FunctionDefinition const& _def, ContractDefinition const& _for
    );

    // Returns the specialization of this method's super call. If no super call
    // exists this throws.
    std::unique_ptr<FunctionSpecialization> super() const;

    // Returns the SmartACE name for the method. Format:
    // - Ctor:             {BASE}_Constructor
    // - Method:           {BASE}_Method_{METHOD}
    // - Modifier:         {BASE}_Modifier_{_depth}_{METHOD}
    // - Fallback:         {BASE}_Fallback
    // - Derived Ctor:     {BASE}_Constructor_For_{DERIVED}
    // - Derived Method:   {BASE}_Method_For_{DERIVED}_{METHOD}
    // - Derived Modifier: {BASE}_Method_{_depth}_For_{DERIVED}_{METHOD}
    std::string name(size_t _depth) const;

    // Returns the contract against which this method is defined.
    ContractDefinition const& source() const;

    // Returns the contract using this method.
    ContractDefinition const& useBy() const;

    // Returns the declaration of this method.
    FunctionDefinition const& func() const;

private:
    // Attempts to extract the contract which declared the method. If the method
    // was not declared on any contract (an error) this throws.
    static ContractDefinition const& get_scope(FunctionDefinition const& _func);

    FunctionDefinition const* M_CALL;
    ContractDefinition const* M_SRC;
    ContractDefinition const* M_USER;
};

// -------------------------------------------------------------------------- //

}
}
}
