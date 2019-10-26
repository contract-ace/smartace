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

// -------------------------------------------------------------------------- //

VariableScopeResolver::VariableScopeResolver(
    bool _instrument
): M_SHADOW(_instrument)
{
}

// -------------------------------------------------------------------------- //

void VariableScopeResolver::enter() { m_scopes.emplace_back(); }

void VariableScopeResolver::exit() { m_scopes.pop_back(); }

// -------------------------------------------------------------------------- //

void VariableScopeResolver::record_declaration(VariableDeclaration const& _decl)
{
    if (!_decl.name().empty())
    {
        m_scopes.back().insert(_decl.name());
    }
}

// -------------------------------------------------------------------------- //

string VariableScopeResolver::resolve_identifier(Identifier const& _id) const
{
    return resolve_sym(_id.name());
}

string VariableScopeResolver::resolve_declaration(
    VariableDeclaration const& _decl
) const
{
    return resolve_sym(_decl.name());
}

// -------------------------------------------------------------------------- //

string VariableScopeResolver::rewrite(string _sym, bool _gen, VarContext _ctx)
{
    ostringstream oss;

    if (!_sym.empty())
    {
        if (_ctx == VarContext::FUNCTION) oss << "func_";

        oss << (_gen ? "model_" : "user_");

        oss << escape_decl_name_string(_sym);
    }

    return oss.str();
}

// -------------------------------------------------------------------------- //

string VariableScopeResolver::resolve_sym(string const& _sym) const
{
    if (_sym.empty()) return _sym;

    for (auto scope = m_scopes.crbegin(); scope != m_scopes.crend(); scope++)
    {
        if (scope->find(_sym) != scope->cend())
        {
            return rewrite(_sym, M_SHADOW, VarContext::FUNCTION);
        }
    }

    if (_sym == "this")
    {
        return "self";
    }
    else if (_sym == "super")
    {
        throw runtime_error("Keyword super not supported.");
    }
    else if (_sym == "block" || _sym == "msg" || _sym == "tx")
    {
        return "state";
    }
    else if (_sym == "now")
    {
        return "state->blocknum";
    }
    else
    {
        return "self->" + rewrite(_sym, M_SHADOW, VarContext::STRUCT);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
