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

CExprPtr FunctionUtilities::try_to_wrap(Type const& _type, CExprPtr _expr)
{
    if (is_wrapped_type(_type))
    {
        string const WRAP = "Init_" + TypeConverter::get_simple_ctype(_type);
        return make_shared<CFuncCall>( WRAP, CArgList{ move(_expr) } );
    }
    return _expr;
}

string FunctionUtilities::name(
    FunctionDefinition const& _def,
    ContractDefinition const& _src,
    ContractDefinition const& _for
)
{
    ostringstream oss;
    oss << "Method_" << escape_decl_name(_src)
        << "_Func" << escape_decl_name(_def);
    if (_src.name() != _for.name())
    {
        oss << "_For_" << escape_decl_name(_for);
    }
    return oss.str();
}

string FunctionUtilities::modifier_name(string _base, size_t _i)
{
    return _base + "_mod" + to_string(_i);
}

string FunctionUtilities::base_name(std::string _base)
{
    return _base + "_base";
}

string FunctionUtilities::ctor_name(
    ContractDefinition const& _src, ContractDefinition const& _for
)
{
    ostringstream oss;
    oss << "Ctor_" << escape_decl_name(_src);
    if (_src.name() != _for.name())
    {
        oss << "_For_" << escape_decl_name(_for);
    }
    return oss.str();
}

}
}
}
