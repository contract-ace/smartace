/*
 * @date 2019
 * This model maps each Solidity type to a C-type. For structures and contracts,
 * these are synthesized C-structs. This translation unit provides utilities for
 * performing such conversions.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <map>
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
 * Maintains a dictionary from AST Node addresses to type annotations. The
 * mapping is restricted to AST Nodes for which types are practical, and records
 * must be generated on a per-source-unit basis.
 */
class TypeConverter : public ASTConstVisitor
{
public:
    // Generates type annotations for all relevant members of _unit. The type
    // annotations may be recovered using the translate method.
    void record(SourceUnit const& _unit);

    // Retrieves a type annotation. This interface is restricted so that only
    // those AST Nodes with annotations may be queried.
    Translation translate(ContractDefinition const& _contract) const;
    Translation translate(StructDefinition const& _structure) const;
    Translation translate(VariableDeclaration const& _decl) const;
    Translation translate(TypeName const& _type) const;
    Translation translate(FunctionDefinition const& _fun) const;
    Translation translate(ModifierDefinition const& _mod) const;

protected:
    bool visit(VariableDeclaration const& _node) override;
	bool visit(ElementaryTypeName const& _node) override;
	bool visit(UserDefinedTypeName const& _node) override;
	bool visit(FunctionTypeName const& _node) override;
    bool visit(Mapping const&) override;
	bool visit(ArrayTypeName const& _node) override;

    void endVisit(ParameterList const& _node) override;
	void endVisit(Mapping const& _node) override;

private:
    std::map<ASTNode const*, Translation> m_dictionary;

    VariableDeclaration const* m_curr_decl = nullptr;
    unsigned int m_rectype_depth = 0;
    bool m_is_retval = false;

    Translation translate_impl(ASTNode const* _node) const;
};

}
}
}
