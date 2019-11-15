/**
 * @date 2019
 * Certain values (ie. CIntLiteral(0), CIntLiteral(1)) are immutable, and used
 * across many points in the code. These values are usually required in heap
 * memory. To reduce the number of instances, this file provides definitive and
 * static locations for all such literals. This is also good, as should the
 * encoding of such literals change, the changes will be reflected across the
 * codebase.
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Core.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Stores all such literals in a single collection.
 */
class Literals
{
public:
    static CExprPtr const ZERO;
    static CExprPtr const ONE;
};

}
}
}
