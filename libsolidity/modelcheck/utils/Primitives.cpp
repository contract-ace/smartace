#include <libsolidity/modelcheck/utils/Primitives.h>

#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

string PrimitiveToRaw::integer(uint16_t _width, bool _signed)
{
    ostringstream oss;
    oss << "sol_raw_" << (_signed ? "" : "u") << "int" << _width << "_t";
    return oss.str();
}

string PrimitiveToRaw::boolean()
{
    return PrimitiveToRaw::integer(8, false);
}

string PrimitiveToRaw::address()
{
    return PrimitiveToRaw::integer(160, false);
}

// -------------------------------------------------------------------------- //

}
}
}
