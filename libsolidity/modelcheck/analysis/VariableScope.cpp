/*
 * @date 2019
 * Provides analysis utilities to determine if a variable is a local (to a
 * function), a member (of a contract), or global (within the EVM).
 */

#include <libsolidity/modelcheck/analysis/VariableScope.h>

#include <libsolidity/modelcheck/utils/CallState.h>
#include <libsolidity/modelcheck/utils/Types.h>
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
    CodeType _code_type
): M_CODE_TYPE(_code_type)
{
}

// -------------------------------------------------------------------------- //

void VariableScopeResolver::assign_spec(FunctionSpecialization const* _spec)
{
    m_spec = _spec;
}

// -------------------------------------------------------------------------- //

FunctionSpecialization const* VariableScopeResolver::spec() const
{
    return m_spec;
}

// -------------------------------------------------------------------------- //

void VariableScopeResolver::enter()
{
    m_scopes.emplace_back();
}

void VariableScopeResolver::exit()
{
    m_scopes.pop_back();
}

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
    if (M_CODE_TYPE == CodeType::INITBLOCK)
    {
        return rewrite(_sym, false, VarContext::STRUCT);
    }

    if (_sym.empty()) return _sym;

    bool shadow = (M_CODE_TYPE == CodeType::SHADOWBLOCK);

    for (auto scope = m_scopes.crbegin(); scope != m_scopes.crend(); scope++)
    {
        if (scope->find(_sym) != scope->cend())
        {
            return rewrite(_sym, shadow, VarContext::FUNCTION);
        }
    }

    if (_sym == "this")
    {
        return "self";
    }
    else if (_sym == "now")
    {
        return CallStateUtilities::get_name(CallStateUtilities::Field::Block);
    }
    else
    {
        return "self->" + rewrite(_sym, shadow, VarContext::STRUCT);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
