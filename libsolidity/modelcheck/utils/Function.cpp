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

string FunctionUtilities::modifier_name(string _base, size_t _i)
{
    return _base + "_mod" + to_string(_i);
}

string FunctionUtilities::base_name(std::string _base)
{
    return _base + "_base";
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
): FunctionSpecialization(_def, get_scope(_def), _for)
{
}

FunctionSpecialization::FunctionSpecialization(
    FunctionDefinition const& _def,
    ContractDefinition const& _src,
    ContractDefinition const& _for
): M_CALL(&_def), M_SRC(&_src), M_USER(&_for)
{
}

unique_ptr<FunctionSpecialization> FunctionSpecialization::super() const
{
    if (auto superfunc = func().annotation().superFunction)
    {
        if (superfunc->isImplemented())
        {
            return make_unique<FunctionSpecialization>(
                *superfunc, get_scope(*superfunc), useby()
            );
        }
    }
    return nullptr;
}

string FunctionSpecialization::name() const
{
    ostringstream oss;

    if (func().isConstructor())
    {
        oss << "Ctor_" << escape_decl_name(source());
    }
    else
    {
        oss << "Method_" << escape_decl_name(source())
            << "_Func" << escape_decl_name(func());
    }

    if (source().name() != useby().name())
    {
        oss << "_For_" << escape_decl_name(useby());
    }

    return oss.str();
}

ContractDefinition const& FunctionSpecialization::source() const
{
    return (*M_SRC);
}

ContractDefinition const& FunctionSpecialization::useby() const
{
    return (*M_USER);
}

FunctionDefinition const& FunctionSpecialization::func() const
{
    return (*M_CALL);
}

ContractDefinition const& FunctionSpecialization::get_scope(
    FunctionDefinition const& _func
)
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
