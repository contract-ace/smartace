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
    void enter_scope(const ContractDefinition &scope);
    void enter_scope(const StructDefinition &scope);
    void enter_scope(const VariableDeclaration &scope);

    // TODO
    void exit_scope();

    // TODO
    Translation translate(const ContractDefinition &datatype) const;
    Translation translate(const StructDefinition &datatype) const;
    Translation translate(const Mapping &datatype) const;

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
