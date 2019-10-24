/*
 * @date 2019
 * Provides analysis utilities to determine if a variable is a local (to a
 * function), a member (of a contract), or global (within the EVM).
 */

#include <libsolidity/modelcheck/VariableScopeResolver.h>

#include <libsolidity/modelcheck/TypeClassification.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

void VariableScopeResolver::enter() { m_scopes.emplace_back(); }

void VariableScopeResolver::exit() { m_scopes.pop_back(); }

void VariableScopeResolver::record_declaration(VariableDeclaration const& _decl)
{
    m_scopes.back().insert(_decl.name());
}

string VariableScopeResolver::resolve_identifier(Identifier const& _id) const
{
    auto const& NAME = _id.name();

    for (auto scope = m_scopes.crbegin(); scope != m_scopes.crend(); scope++)
    {
        auto const& MATCH = scope->find(NAME);
        if (MATCH != scope->cend()) return NAME;
    }

    if (NAME == "this")
    {
        return "self";
    }
    else if (NAME == "super")
    {
        throw runtime_error("Keyword super not supported.");
    }
    else if (NAME == "block" || NAME == "msg" || NAME == "tx")
    {
        return "state";
    }
    else if (NAME == "now")
    {
        return "state->blocknum";
    }
    else
    {
        return "self->d_" + NAME;
    }
}

string VariableScopeResolver::rewrite(string _sym, bool _gen, VarContext _ctx)
{
    ostringstream oss;

    if (_ctx == VarContext::FUNCTION) oss << "func_";
    else if (_ctx == VarContext::MODIFIER) oss << "mod_";

    oss << (_gen ? "model_" : "user_");

    oss << escape_decl_name_string(_sym);

    return oss.str();
}

}
}
}
