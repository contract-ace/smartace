/**
 * @date 2019
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
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
    // The fields used by a map.
    static const std::string SET_FIELD;
    static const std::string CURR_FIELD;
    static const std::string DATA_FIELD;
    static const std::string ND_FIELD;

    // Constructs a new map. The map models AST node _src. The map will model
    // _ct entries. Its key and value types are converted using _converter,
    // along iwth the map itself.
    MapGenerator(
        Mapping const& _src, size_t _ct, TypeConverter const& _converter)
    ;

    // Declares all structures and functions used by a map.
    CStructDef declare(bool _forward_declare) const;
    CFuncDef declare_zero_initializer(bool _forward_declare) const;
    CFuncDef declare_nd_initializer(bool _forward_declare) const;
    CFuncDef declare_write(bool _forward_declare) const;
    CFuncDef declare_read(bool _forward_declare) const;
    CFuncDef declare_ref(bool _forward_declare) const;

private:
    // Static literals for true and false values.
    static std::shared_ptr<CIntLiteral> const TRUE;
    static std::shared_ptr<CIntLiteral> const FALSE;

    // The number of elements modeling the map.
    size_t const M_LEN;

    // The member types for the map.
    std::string const M_NAME;
    std::string const M_TYPE;
    std::string const M_KEY_T;
    std::string const M_VAL_T;

    // Utility calls for map initialization.
    CExprPtr const M_INIT_KEY;
    CExprPtr const M_INIT_VAL;
    CExprPtr const M_ND_VAL;

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
    static CStmtPtr expand_iteration(
        size_t _i,
        CVarDecl const& _arr,
        CVarDecl const& _key,
        CStmtPtr _exec,
        CStmtPtr _chain
    );

    // Helper functions to generate read/write requests to indexed fields.
    static std::string field(std::string const& _base, size_t _i);
    static CStmtPtr write_field_stmt(
        CData const& _base, std::string const& _field, size_t _i, CExprPtr _data
    );
};

}
}
}
