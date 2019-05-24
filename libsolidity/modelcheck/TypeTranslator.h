/*
 * TODO
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
 *
 */
struct Translation
{
    std::string type;
    std::string name;
};

/*
 * TODO
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

    // TODO
    Translation translate(ContractDefinition const& datatype) const;
    Translation translate(StructDefinition const& datatype) const;
    Translation translate(Mapping const& datatype) const;
    Translation translate(TypeName const& datatype) const;

    // TODO
    Translation scope() const;

private:
    boost::optional<std::string> m_contract_ctx;
    boost::optional<std::string> m_struct_ctx;
    boost::optional<std::string> m_map_ctx;
};

}
}
}
