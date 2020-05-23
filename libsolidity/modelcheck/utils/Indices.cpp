/**
 * @date 2020
 * Data and helper functions for generating indices.
 */

#include <libsolidity/modelcheck/utils/Indices.h>

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

string Indices::const_global_name(dev::u256 _var)
{
    ostringstream oss;
    oss << "global_index_const_" << _var;
    return oss.str();
}

// -------------------------------------------------------------------------- //

}
}
}
