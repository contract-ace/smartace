/**
 * @date 2019
 * A collection of mostly free functions, which allow for categorizing AST nodes
 * into C-struct groups. These are direct translations of the type analysis
 * specifications.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <string>

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
 * This operation realize `tau_{type}` from the translation specifications.
 */
bool has_simple_type(Declaration const& _node);
bool has_simple_type(TypeName const& _node);
bool has_simple_type(Expression const& _node);

/**
 * Extracts the name from a declaration. The name is rewritten such that an odd
 * run of underscores will never occur. This is done such that if A.name()
 * equals B.name() if and only if escape_decl_name(A) = escape_decl_name(B).
 * 
 * This operation realizes `name` from the translation specifications.
 */
std::string escape_decl_name(Declaration const& _decl);

}
}
}
