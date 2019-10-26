/**
 * @date 2019
 * Data and helper functions for generating functions. This is meant to reduce
 * code duplication.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Utilities to move a function from the Solidity representation to the c-model
 * representation.
 */
class FunctionUtilities
{
public:
    // Returns the root node of a function. This is defined as the ASTNode which
    // should be "called" when the function is invoked.
    static ASTNode const& extract_root(FunctionDefinition const& _func);
};

}
}
}
