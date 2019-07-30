/**
 * @date 2019
 * A collection of mostly free functions, which allow for categorizing AST nodes
 * into C-struct groups. These are direct translations of the type analysis
 * specifications.
 */

#pragma once

#include <libsolidity/ast/AST.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Returns true if _type is a simple type, as opposed to a compound type. 
 */
bool is_simple_type(Type const& _type);

/**
 * Extracts the type associated with an AST node, and returns true if it is a
 * simple type. This is equivalent to extracting the type manually, and then
 * invoking is_simple_type.
 * 
 * This operation realize tau_{type}.
 */
bool has_simple_type(Declaration const& _node);
bool has_simple_type(TypeName const& _node);
bool has_simple_type(Expression const& _node);

}
}
}
