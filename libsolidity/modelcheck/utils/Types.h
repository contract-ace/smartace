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
 * Unwraps the TypeType, and converts RationalNumberType to a storage-based
 * type.
 */
Type const& unwrap(Type const& _type);

/*
 * Determines the number of bits needed to represent _type.
 */
int simple_bit_count(Type const& _type);

/*
 * Determines if _type uses a signed type.
 */
bool simple_is_signed(Type const& _type);

/**
 * Returns true if _type is a wrapped, primitive data-type.
 */
bool is_wrapped_type(Type const& _expr);

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
 * See escape_decl_name: escape_decl_name(A) = escape_decl_name_string(A.name())
 */
std::string escape_decl_name_string(std::string const& _name);

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
