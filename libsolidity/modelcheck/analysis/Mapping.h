/**
 * Provides utilities for summarizing map declarations, and flattening those
 * declarations into single mappings with multi-dimensional keys.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <list>
#include <map>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Simple utility to extract mappings from a list of variable declarations.
 */
class MappingExtractor : public ASTConstVisitor
{
public:
    // Creates an empty extractor.
    MappingExtractor() = default;

    // Extracts all mappings from _vars.
    explicit
        MappingExtractor(std::vector<ASTPointer<VariableDeclaration>> _vars);

    // Manually analyzes _var, to aggregate mappings across multiple lists.
    void record(VariableDeclaration const* _var);

    // Returns the extracted mappings.
    std::list<Mapping const*> get() const;

protected:
	bool visit(Mapping const& _node) override;

private:
    std::list<Mapping const*> m_mappings;
};

// -------------------------------------------------------------------------- //

/**
 * Not quite MapReduce... This class consumes the AST for `mapping` typenames,
 * and constructs a database mapping said typenames to (1) an ordered list of
 * keys in the map and (2) the type of the map within the encoding.
 */
class MapDeflate
{
public:
    // Fully describes the flattened map. See MapDeflate description.
    struct FlatMap
    {
        std::string name;
        std::vector<ElementaryTypeName const*> key_types;
        TypeName const* value_type;
    };
    using Record = std::shared_ptr<FlatMap const>;

    // Either registers _map with the MapDeflate lookup, or returns the pre-
    // existing entry. When no entry is found, this flatten _map, and registers
    // it in the map lookup. For a Mapping typename of the form
    // mapping(A => mapping(B => mapping(C => ...))). a FlatMap with key
    // tuple(A, B, C, ...) will be produced.
    Record query(Mapping const& _mapping);

    // Queries the FlatMap for the given declaration, and returns a nullptr on
    // failed lookup.
    Record resolve(Mapping const& _dcl) const;
    Record resolve(VariableDeclaration const& _decl) const;

    // Queries the FlatMap for the given declaration, and throws an exception on
    // failed lookup.
    Record try_resolve(Mapping const& _decl) const;
    Record try_resolve(VariableDeclaration const& _dcl) const;

private:
    std::map<Mapping const*, std::shared_ptr<FlatMap>> m_flatset;
};

// -------------------------------------------------------------------------- //

/**
 * Analyzers an IndexAccess, flattens all keys into a list, and extracts the
 * base map to access.
 */
class FlatIndex: public ASTConstVisitor
{
public:
    // Extracts all access information wrt _root.
    FlatIndex(IndexAccess const& _root);

    // Returns a flattened list of indices to the map.
    std::list<Expression const*> const& indices() const;

    // Returns the expression which designates the map.
    Expression const& base() const;

    // Returns the declaration referenced by this index.
    VariableDeclaration const& decl() const;

protected:
	bool visit(Conditional const&) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(IndexAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
    Expression const* m_base = nullptr;
    VariableDeclaration const* m_decl = nullptr;

    std::list<Expression const*> m_indices;
};

// -------------------------------------------------------------------------- //

}
}
}
