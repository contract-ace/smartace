/**
 * Provides analysis utilities to determine the SmartACE identifiers which map
 * to Solidity identifiers.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/AST.h>

#include <set>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class FunctionSpecialization;

// -------------------------------------------------------------------------- //

enum class VarContext { STRUCT, FUNCTION };
enum class CodeType { SOLBLOCK, SHADOWBLOCK, INITBLOCK };

/**
 * Takes into account the current code context, and uses this information to map
 * Solidity identifiers to their cooresponding SmartACE identifiers.
 */
class VariableScopeResolver
{
public:
    // Generates a mapping from Solidity identifiers, to SmartACE identifiers,
    // given the Solidity Identifier came from the context of _code_type.
    VariableScopeResolver(CodeType _code_type = CodeType::SOLBLOCK);

    // Associates the scope with some contract scope.
    void assign_spec(FunctionSpecialization const* _spec);

    // Returns all specialization data for the current function scope.
    FunctionSpecialization const* spec() const;

    // Creates or destroys a variable scope.
    void enter();
    void exit();

    // Records a variable declaration in the top-most scope.
    void record_declaration(VariableDeclaration const& _decl);

    // Maps an identifer to its C-model name in the present scope.
    std::string resolve_identifier(Identifier const& _id) const;
    std::string resolve_declaration(VariableDeclaration const& _decl) const;

    // Automatically rewrites identifier names, to avoid variable aliasing. A
    // rewrite has form ("func_","")("client_","model_")escape(_sym). This
    // allows for disambiguating between...
    // 1. modifier variables and function variables after inlining.
    // 2. state variables and local variables.
    // 3. code variables and instrumented variables.
    static std::string rewrite(std::string _sym, bool _gen, VarContext _ctx);

private:
    CodeType const M_CODE_TYPE;

    FunctionSpecialization const* m_spec;

    std::vector<std::set<std::string>> m_scopes;

    // Consumes the string representation of an identifier, _sym, and maps it to
    // a SmartACE identifier.
    std::string resolve_sym(std::string const& _sym) const;
};

// -------------------------------------------------------------------------- //

}
}
}
