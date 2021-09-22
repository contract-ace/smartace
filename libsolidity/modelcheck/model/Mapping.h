/**
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/codegen/Details.h>

#include <memory>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class TypeAnalyzer;
class NondetSourceRegistry;

// -------------------------------------------------------------------------- //

/**
 * Converts Solidity mappings into SmartACE C structs and C functions.
 */
class MapGenerator
{
public:
    // Constructs a new map. The map models AST node _src. The map will model
    // _ct entries. Its key and value types are converted using _converter,
    // along with the map itself. If _keep_sum is set and if the map's values
    // have a simple type, the sum aggregator is instrumented by default.
    MapGenerator(
        Mapping const& _src,
        bool _keep_sum,
        size_t _ct,
        TypeAnalyzer const& _converter
    );

    // Declares all structures and functions used by a map.
    CStructDef declare(bool _forward_declare) const;
    CFuncDef declare_zero_initializer(bool _forward_declare) const;
    CFuncDef declare_write(bool _forward_declare) const;
    CFuncDef declare_read(bool _forward_declare) const;

private:
    // The number of elements modeling the map.
    size_t const M_LEN;
    std::string const M_TYPE;

    // Allows types to be resolved.
    TypeAnalyzer const& M_CONVERTER;
    std::shared_ptr<MapDeflate::FlatMap const> const M_MAP_RECORD;

    // Maintain sum of values in maps of simple types.
    bool const M_KEEP_SUM;

    // Const type names to simplify generation.
    std::string const M_VAL_T;

    // Const members to simplify generation and facilitate reuse.
    std::shared_ptr<CVarDecl> const M_TMP;
    std::shared_ptr<CVarDecl> const M_ARR;
    std::shared_ptr<CVarDecl> const M_DAT;

    // Key fields.
    std::vector<std::shared_ptr<CVarDecl>> m_keys;

    // Generate the (_depth)-th block in either a read or write method, for a
    // nested mapping. _suffix is used to identify the key. _is_writer
    // distinguishes reads from writes. _maintain_sum specifies whether or not
    // a sum variable should be updated on write.
    CStmtPtr expand_access(
        size_t _depth,
        std::string const& _suffix,
        bool _is_writer,
        bool _maintain_sum
    ) const;
};

// -------------------------------------------------------------------------- //

}
}
}
