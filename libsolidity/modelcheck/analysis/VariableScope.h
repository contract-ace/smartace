/*
 * @date 2019
 * Provides analysis utilities to determine if a variable is a local (to a
 * function), a member (of a contract), or global (within the EVM).
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <list>
#include <set>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

enum class VarContext { STRUCT, FUNCTION };
enum class CodeType { SOLBLOCK, SHADOWBLOCK, INITBLOCK };

/*
 * Maintains a hierarchy of scopes and their declaration names. Allows variable
 * names to be mapped to their names within the C-model, given the present
 * scope.
 */
class VariableScopeResolver
{
public:
    // When false, the variable scope maps to user variables. Otherwise, acts as
    // a shadow scope for instrumentation variables.
    VariableScopeResolver(CodeType _code_type = CodeType::SOLBLOCK);

    // Creates or destroys a variable scope.
    void enter();
    void exit();

    // Records a variable declaration in the top-most scope.
    void record_declaration(VariableDeclaration const& _decl);

    // Maps an indentifer to its C-model name in the present scope.
    std::string resolve_identifier(Identifier const& _id) const;
    std::string resolve_declaration(VariableDeclaration const& _decl) const;

    // Automatically rewrites identifier names, to avoid variable aliasing. A
    // rewrite has form ("func_","mod_","")("client_","model_")escape(_sym).
    // This allows for disambiguating between...
    // 1. modifier variables and function variables after inlining.
    // 2. local variables and global symbols (function names, etc).
    // 3. client variables and tooling generated instrumentation variables.
    static std::string rewrite(std::string _sym, bool _gen, VarContext _ctx);

private:
    CodeType const M_CODE_TYPE;

    std::list<std::set<std::string>> m_scopes;

    // Resolves any string within the resolver.
    std::string resolve_sym(std::string const& _sym) const;
};

}
}
}
