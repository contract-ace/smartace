/**
 * @date 2019
 * Certain values (ie. CIntLiteral(0), CIntLiteral(1)) are immutable, and used
 * across many points in the code. These values are usually required in heap
 * memory. To reduce the number of instances, this file provides definitive and
 * static locations for all such literals. This is also good, as should the
 * encoding of such literals change, the changes will be reflected across the
 * codebase.
 */

#include <libsolidity/modelcheck/TranslationLiterals.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <memory>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

CExprPtr const Literals::ZERO = make_shared<CIntLiteral>(0);
CExprPtr const Literals::ONE = make_shared<CIntLiteral>(1);

}
}
}
