/**
 * @date 2019
 * Data and helper functions for generating functions. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/Function.h>

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

}
}
}
