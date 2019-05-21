/**
 * @date 2019
 * Utility visitor to compute the dimensionality of a solidity map. This can
 * be applied to the top-level of a map, or to a partially accessed map.
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
): m_map(&_map)
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
