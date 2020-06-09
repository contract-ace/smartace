/**
 * Data and helper functions for generating functions. This is meant to reduce
 * code duplication.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/codegen/Details.h>

#include <memory>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class TypeAnalyzer;

// -------------------------------------------------------------------------- //

/**
 * Returns true if _f1 and _f2 can collid. That is, if the two functions share
 * names and argument types.
 */
bool collid(FunctionDefinition const& _f1, FunctionDefinition const& _f2);

// -------------------------------------------------------------------------- //

/**
 * This class manages calls to, and specializations of, initialization calls. In
 * the SmartACE model we distinguish constructors from initializers. All types
 * have initializers, which follow the convention `Init_{TYPENAME}`, or in the
 * case of derived contract initializers, `INIT_{BASE_NAME}_For_{DERIVED_NAME}`.
 */
class InitFunction
{
public:
    // Represents an initialization call for _base, as specialized for _derived.
    // Type names are resolved using _converter.
    InitFunction(
        TypeAnalyzer const& _converter,
        ContractDefinition const& _base,
        ContractDefinition const& _derived
    );

    // Represents an initialization call for entities of _node's type. Type
    // names are resolved using _converter.
    InitFunction(TypeAnalyzer const& _converter, ASTNode const& _node);

    // Represents an initialization call for the mapping built from _mapping.
    // Type names are resolved using _converter.
    InitFunction(MapDeflate::FlatMap const& _mapping);

    // An initialization call for a structure type-defined as _type.
    InitFunction(std::string _type);

    // Returns a call builder, configured for the standard initializer.
    CFuncCallBuilder call_builder() const;

    // Returns the variable declaration for the standard initializer function def.
    std::shared_ptr<CVarDecl> call_id() const;

    // Returns the name of the standard initializer.
    std::string call_name() const;

    // Returns a call to the zero initializer.
    CExprPtr defaulted() const;

    // Returns the variable declaration for the zero initializer function def.
    std::shared_ptr<CVarDecl> default_id() const;

    // Returns the name of the zero initializer.
    std::string default_name() const;

    // For a simple type, _type, this returns a call to the initializer of _type.
    // against _expr. Otherwise, _expr is returned unmodified.
    static CExprPtr wrap(Type const& _type, CExprPtr _expr);

    // The variable name reserved for initializers to set return values by ref.
    static std::string const INIT_VAR;

private:
    // The prefixes used to distinguish initialization calls, i.e., `Init_`.
    static std::string const PREFIX;
    static std::string const DEFAULT_PREFIX;

    std::string const M_NAME;
    std::string const M_TYPE;

    // Internal function to set _name and _type.
    InitFunction(std::string _name, std::string _type);

    // Wrapping call to make_shared<CVarDecl>(M_TYPE, _name).
    std::shared_ptr<CVarDecl> make_id(std::string _name) const;

    // Returns `Init_{BASE_NAME}`, or `Init_{BASE_NAME}_For_{DERIVED_NAME}` when
    // _base is not _derived.
    static std::string specialize_name(
        TypeAnalyzer const& _converter,
        ContractDefinition const& _base,
        ContractDefinition const& _derived
    );
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
    ContractDefinition const& use_by() const;

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
