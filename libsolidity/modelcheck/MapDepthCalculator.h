/**
 * @date 2019
 * Utility visitor to compute the dimensionality of a solidity map. This can
 * be applied to the top-level of a map, or to a partially accessed map.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * A class which encapsulates a map, and calculates depth-related metrics.
 */
class MapDepthCalculator : public ASTConstVisitor
{
public:
    // Binds the calculator to the given AST map node.
    MapDepthCalculator(
        Mapping const& _map
    );

    // Computes the dimensionality of the map.
    unsigned int depth();

    bool visit(Mapping const& _node) override;

private:
    Mapping const* m_map;

    unsigned int m_depth{0};
};

}
}
}
