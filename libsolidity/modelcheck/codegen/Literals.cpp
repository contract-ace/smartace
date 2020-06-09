#include <libsolidity/modelcheck/codegen/Literals.h>

#include <libsolidity/modelcheck/codegen/Details.h>
#include <memory>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CExprPtr const Literals::ZERO = make_shared<CIntLiteral>(0);
CExprPtr const Literals::ONE = make_shared<CIntLiteral>(1);

// -------------------------------------------------------------------------- //

}
}
}
