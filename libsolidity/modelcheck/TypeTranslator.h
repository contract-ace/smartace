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
 * TODO
 */
class TypeTranslator
{
public:
    // TODO
    void enter_scope(const ContractDefinition &scope);
    void enter_scope(const StructDefinition &scope);
    void enter_scope(const VariableDeclaration &scope);

    // TODO
    void exit_scope();

    // TODO
    std::string translate(const ContractDefinition &datatype) const;
    std::string translate(const StructDefinition &datatype) const;
    std::string translate(const Mapping &datatype) const;

private:
    boost::optional<std::string> m_contract_ctx;
    boost::optional<std::string> m_struct_ctx;
    boost::optional<std::string> m_map_ctx;
};

}
}
}
