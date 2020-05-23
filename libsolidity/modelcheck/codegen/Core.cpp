/**
 * @date 2019
 * Declares the core interface to SimpleCGenerator.cpp
 */

#include <libsolidity/modelcheck/codegen/Core.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ostream & operator<<(ostream & _out, CElement const& _comp)
{
    _comp.print(_out);
    return _out;
}

// -------------------------------------------------------------------------- //

bool CExpr::is_pointer() const
{
    return false;
}

// -------------------------------------------------------------------------- //

void CStmt::nest()
{
    m_is_nested = true;
}

void CStmt::print(ostream & _out) const
{
    print_impl(_out);
    if (!m_is_nested) _out << ";";
}

// -------------------------------------------------------------------------- //

}
}
}
