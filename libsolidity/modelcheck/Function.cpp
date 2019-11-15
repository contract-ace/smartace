/**
 * @date 2019
 * Data and helper functions for generating functions. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/Function.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/TypeClassification.h>
#include <libsolidity/modelcheck/TypeTranslator.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

ASTNode const& FunctionUtilities::extract_root(FunctionDefinition const& _func)
{
    if (_func.modifiers().empty()) return _func;
    return *_func.modifiers()[0];
}

CExprPtr FunctionUtilities::try_to_wrap(Type const& _type, CExprPtr _expr)
{
    if (is_wrapped_type(_type))
    {
        string const WRAP = "Init_" + TypeConverter::get_simple_ctype(_type);
        return make_shared<CFuncCall>( WRAP, CArgList{ move(_expr) } );
    }
    return _expr;
}

}
}
}
