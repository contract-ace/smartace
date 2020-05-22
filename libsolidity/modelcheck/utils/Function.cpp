/**
 * @date 2019
 * Data and helper functions for generating functions. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/utils/Function.h>

#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/codegen/Details.h>
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

CExprPtr FunctionUtilities::try_to_wrap(Type const& _type, CExprPtr _expr)
{
    if (is_wrapped_type(_type))
    {
        string const WRAP = "Init_" + TypeConverter::get_simple_ctype(_type);
        return make_shared<CFuncCall>( WRAP, CArgList{ move(_expr) } );
    }
    return _expr;
}

string FunctionUtilities::init_var()
{
    return "dest";
}

// -------------------------------------------------------------------------- //

FunctionSpecialization::FunctionSpecialization(
    FunctionDefinition const& _def
): FunctionSpecialization(_def, get_scope(_def))
{
}

FunctionSpecialization::FunctionSpecialization(
    FunctionDefinition const& _def, ContractDefinition const& _for
): M_CALL(&_def), M_SRC(&get_scope(_def)), M_USER(&_for)
{
}

unique_ptr<FunctionSpecialization> FunctionSpecialization::super() const
{
    if (auto superfunc = func().annotation().superFunction)
    {
        if (superfunc->isImplemented())
        {
            return make_unique<FunctionSpecialization>(*superfunc, useBy());
        }
    }
    return nullptr;
}

string FunctionSpecialization::name(size_t _depth) const
{
    ostringstream oss;

    // Prefixed by name of source contract.
    oss << escape_decl_name(*M_SRC) << "_";

    // Determines the type of method.
    bool is_method = false;
    if (M_CALL->isConstructor())
    {
        oss << "Constructor";
    }
    else if (M_CALL->isFallback())
    {
        oss << "Fallback";
    }
    else
    {
        oss << "Method";
        is_method = true;
    }
    
    // Appends the modifier index if this is not the entry method.
    if (_depth > 0) oss << "_" << _depth;

    // Adds an override specifier, for when M_CALL is specialized for M_USER.
    if (M_SRC != M_USER)
    {
        oss << "_For_" << escape_decl_name(*M_USER);
    }

    // Adds method name if  this is a named method.
    if (is_method)
    {
        oss << "_" << escape_decl_name(*M_CALL);
    }

    return oss.str();
}

ContractDefinition const& FunctionSpecialization::source() const
{
    return (*M_SRC);
}

ContractDefinition const& FunctionSpecialization::useBy() const
{
    return (*M_USER);
}

FunctionDefinition const& FunctionSpecialization::func() const
{
    return (*M_CALL);
}

ContractDefinition const&
    FunctionSpecialization::get_scope(FunctionDefinition const& _func)
{
    auto scope = dynamic_cast<ContractDefinition const*>(_func.scope());
    if (!scope)
    {
        throw runtime_error("Detected FunctionDefinition without scope.");
    }
    return *scope;
}

// -------------------------------------------------------------------------- //

}
}
}
