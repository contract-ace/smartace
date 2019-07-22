/**
 * @date 2019
 * Declares the core interface to SimpleCGenerator.cpp
 */

#include <libsolidity/modelcheck/SimpleCCore.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

ostream & operator<<(ostream & _out, CElement const& _comp)
{
    _comp.print(_out);
    return _out;
}

bool CExpr::is_pointer() const
{
    return false;
}

}
}
}
