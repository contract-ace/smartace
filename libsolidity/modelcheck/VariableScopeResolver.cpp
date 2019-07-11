/*
 * @date 2019
 * Provides analysis utilities to determine if a variable is a local (to a
 * function), a member (of a contract), or global (within the EVM).
 */

#include <libsolidity/modelcheck/VariableScopeResolver.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

void VariableScopeResolver::enter()
{
    m_scopes.emplace_back();
}

void VariableScopeResolver::exit()
{
    m_scopes.pop_back();
}

void VariableScopeResolver::record_declaration(VariableDeclaration const& _decl)
{
    m_scopes.back().insert(_decl.name());
}

string VariableScopeResolver::resolve_identifier(Identifier const& _id) const
{
    auto const& name = _id.name();

    for (auto scope = m_scopes.crbegin(); scope != m_scopes.crend(); scope++)
    {
        auto const& match = scope->find(name);
        if (match != scope->cend())
        {
            return name;
        }
    }

    if (name == "this")
    {
        return "self";
    }
    else if (name == "super")
    {
        throw runtime_error("Keyword super not supported.");
    }
    else if (name == "block" || name == "msg" || name == "tx")
    {
        return "state";
    }
    else if (name == "now")
    {
        return "state->blocknum";
    }
    else
    {
        return "self->d_" + name;
    }
}

}
}
}
