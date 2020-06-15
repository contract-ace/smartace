#include <libsolidity/modelcheck/utils/Named.h>

#include <libsolidity/ast/AST.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

Named::Named(Declaration const& _decl) : M_NAME(_decl.name()) {}

string Named::name() const { return M_NAME; }

// -------------------------------------------------------------------------- //

}
}
}
