/*
 * This model maps each Solidity type to a C-type. For structures and contracts,
 * these are synthesized C-structs. This translation unit provides utilities for
 * performing such conversions.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <string>
#include <boost/optional.hpp>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/*
 * Provides the name and fully-qualified type of a C-model translation. For
 * primitive types, the name and fully-qualified type will be the same.
 */
struct Translation
{
    std::string type;
    std::string name;
};

/*
 * Converts types and AST nodes into C-model translations.
 */
class TypeTranslator
{
public:
    // TODO
    void enter_scope(ContractDefinition const& scope);
    void enter_scope(StructDefinition const& scope);
    void enter_scope(VariableDeclaration const& scope);

    // TODO
    void exit_scope();

    // Generalized hooks for translating AST data structures.
    Translation translate(Declaration const& datatype) const;
    Translation translate(TypeName const& datatype) const;
    Translation translate(TypePointer t) const;

    // TODO
    Translation scope() const;

private:
    // Translation for categories with 1-to-1 mappings to C primitives. This
    // assumes `name` is a C primitive, and will use that as the translation.
    Translation translate_basic(const std::string &name) const;
    // Validates t is an IntType, and then handles signedness.
    Translation translate_int(TypePointer t) const;
    // Validates t is a contract, and then translates.
    Translation translate_contract(TypePointer t) const;
    // Validates t is a structure, and then resolves scope before translation.
    // Failure to resolve scope will result in an exception. The result will be
    // of the form <scope.name()>_<struct.name()>.
    Translation translate_struct(TypePointer t) const;
    // Validates t is a mapping, resolves the the depth of this map (or submap),
    // and then produces a C type. The result will be of the form,
    // <scope.name()>_<map.name()>_submap<depth>
    Translation translate_mapping(TypePointer t) const;
    // Unwraps TypeTypes.
    Translation translate_type(TypePointer t) const;

    boost::optional<std::string> m_contract_ctx;
    boost::optional<std::string> m_struct_ctx;
    boost::optional<std::string> m_map_ctx;
};

}
}
}
