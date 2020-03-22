/**
 * @date 2019
 * Data and helper functions for generating CallState arguments.
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

// -------------------------------------------------------------------------- //

/**
 * Utilities to generate fields from argument types.
 */
class CallStateUtilities
{
public:
    // An enum for supported call state variables.
    enum class Field { Sender, Origin, Value, Block, Timestamp, Paid };

    // Maps a magic type to a field.
    static Field parse_magic_type(Type const& _type, std::string _field);

    // Resolves a name from a type.
    static std::string get_name(Field _field);

    // Resolves a model type from a field type.
    static Type const* get_type(Field _field);

private:
    // Static mapping from magic types to field names.
    static std::map<std::pair<MagicType::Kind, std::string>, Field> const
        MAGIC_TYPE_LOOKUP;

    // Static type instances to reference in return values
    static AddressType const SENDER_TYPE;
    static IntegerType const COUNTABLE_TYPE;
    static BoolType const BOOLEAN_TYPE;
};

// -------------------------------------------------------------------------- //

}
}
}
