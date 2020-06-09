/**
 * Data and helper functions for generating indices.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/utils/AbstractAddressDomain.h>

#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
    
// -------------------------------------------------------------------------- //

string AbstractAddressDomain::literal_name(dev::u256 _var)
{
    ostringstream oss;
    oss << "g_literal_address_" << _var;
    return oss.str();
}

// -------------------------------------------------------------------------- //

}
}
}
