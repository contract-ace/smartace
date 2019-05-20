/*
 * TODO
 */

#include <libsolidity/modelcheck/MapDepthCalculator.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

MapDepthCalculator::MapDepthCalculator(
    Mapping const& _map
) : m_map(&_map)
{
}

unsigned int MapDepthCalculator::depth()
{
    if (m_depth == 0)
    {
        m_map->accept(*this);
    }
    return m_depth;
}

bool MapDepthCalculator::visit(Mapping const&)
{
    ++m_depth;
    return true;
}

}
}
}
