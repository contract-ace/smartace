/**
 * @date 2019
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <memory>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * A collection of mapping-related utilities.
 */
class MappingUtilities
{
public:
};

class MapGenerator
{
public:
    // Constructs a new map. The map models AST node _src. The map will model
    // _ct entries. Its key and value types are converted using _converter,
    // along iwth the map itself.
    MapGenerator(
        Mapping const& _src, size_t _ct, TypeConverter const& _converter
    );

    // Declares all structures and functions used by a map.
    CStructDef declare(bool _forward_declare) const;
    CFuncDef declare_zero_initializer(bool _forward_declare) const;
    CFuncDef declare_nd_initializer(bool _forward_declare) const;
    CFuncDef declare_write(bool _forward_declare) const;
    CFuncDef declare_read(bool _forward_declare) const;

    // Generates a name for each global index.
    static std::string name_global_key(size_t _id);

private:
    // Utility to iterate all key combinations.
    class KeyIterator
    {
    public:
        // Creates an iterator over all length 1 to length _depth strings over
        // the alphabet [0, 1, ..., _width-1].
        KeyIterator(size_t _width, size_t _depth);

        // Returns the current string as (s_0)_(s_2)_..._(s_{i}), where i is
        // some length from 1 to _depth. This value changes only after a call to
        // next.
        std::string suffix() const;

        // Returns true if the current suffix is of maximum length.
        bool is_full() const;

        // Returns the symbol most recently appended to the suffix.
        size_t top() const;

        // Returns the current suffix length.
        size_t size() const;

        // Updates all view fields. Returns true if there is a suffix left to iterate.
        bool next();
    
    private:
        // The dimensions of the search space.
        size_t M_WIDTH;
        size_t M_DEPTH;

        // The current string.
        std::list<size_t> m_indices;
    };

    // The number of elements modeling the map.
    size_t const M_LEN;
    std::string const M_TYPE;

    // Allows types to be resolved.
    TypeConverter const& M_TYPES;
    MapDeflate::FlatMap const M_MAP_RECORD;

    // Const type names to simplify generation.
    std::string const M_VAL_T;

    // Const members to simplfy generation and facilitate reuse.
    std::shared_ptr<CVarDecl> const M_TMP;
    std::shared_ptr<CVarDecl> const M_ARR;
    std::shared_ptr<CVarDecl> const M_DAT;

    // Key fields.
    std::vector<std::string> m_keys_t;
    std::vector<std::shared_ptr<CVarDecl>> m_keys;

    // Helper utilities to generate in-place loop "iteration". The {name}{i}
    // member can be rationalized at the {name} structure of the {i}-th element
    // in an array of structures. The resulting if-else structures correspond to
    // iterating over such a structure. expand_init unrolls the initialization
    // loop with each data field set to _init_data. expand_iteration unrolls the
    // _i-th iteration of a search loop, where the search is for an _i such that
    // _key == curr{i}. If an empty slot is found, that index is used instead.
    // In either case, once a match has been found, _exec is inlined. _last is
    // taken to be either null, or the {i-1}-th term.
    std::shared_ptr<CBlock> expand_init(CExprPtr const& _init_data) const;

    CStmtPtr expand_access(
        size_t _depth, std::string const& _suffix, bool _is_writer
    ) const;
};

}
}
}
