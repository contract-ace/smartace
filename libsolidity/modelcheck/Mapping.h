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
    MapGenerator(Mapping const& _src, int _ct, TypeConverter const& _converter);

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
    int const M_LEN;

    // The member types for the map.
    std::string const M_NAME;
    std::string const M_TYPE;
    std::string const M_KEY_TYPE;
    std::string const M_VAL_TYPE;

    // Utility calls for map initialization.
    CExprPtr const M_INIT_KEY;
    CExprPtr const M_INIT_VAL;
    CExprPtr const M_ND_KEY;
    CExprPtr const M_ND_VAL;

    // Important "magic values".
    CExprPtr const M_LEN_LITERAL;

    // Utility functions to generate map iteration code.
    std::shared_ptr<CVarDecl> generate_loop_idx() const;
    std::shared_ptr<CForLoop> expand_loop_template(
        std::shared_ptr<CBlock> _body, bool _new_idx
    ) const;
    std::shared_ptr<CForLoop> expand_array_search() const;

    // Generates function parameters.
    std::shared_ptr<CVarDecl> make_tmp() const;
    std::shared_ptr<CVarDecl> make_arr() const;
    std::shared_ptr<CVarDecl> make_key() const;
    std::shared_ptr<CIndexAccess> read_tmp_field(std::string _field) const;
    std::shared_ptr<CIndexAccess> read_arr_field(std::string _field) const;
};

}
}
}
