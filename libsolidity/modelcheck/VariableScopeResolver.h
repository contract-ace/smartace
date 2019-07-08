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

/*
 * Maintains a hierarchy of scopes and their declaration names. Allows variable
 * names to be mapped to their names within the C-model, given the present
 * scope.
 */
class VariableScopeResolver
{
public:
    // Creates or destroys a variable scope.
    void enter();
    void exit();

    // Records a variable declaration in the top-most scope.
    void record_declaration(VariableDeclaration const& _decl);

    // Maps an indentifer to its C-model name in the present scope.
    std::string resolve_identifier(Identifier const& _id) const;

private:
    std::list<std::set<std::string>> m_scopes;
};

}
}
}
