/**
 * SmartACE maps every Solidity type to some generated C-type. For primitive
 * types, these are singleton structures. For Solidity structures and contracts,
 * these are synthesized C-structs. These utilities allow for translation from
 * Solidity to C.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/codegen/Core.h>

#include <map>
#include <set>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallGraph;

// -------------------------------------------------------------------------- //

/*
 * Maintains a dictionary from AST Node addresses to type encoding metadata. The
 * mapping is restricted to AST Nodes for which types are practical, and records
 * must be generated on a per-source-unit basis.
 */
class TypeAnalyzer : public ASTConstVisitor
{
public:
    //
    TypeAnalyzer(std::vector<SourceUnit const*> _units, CallGraph const& _calls);

    // Returns the CType used to model _node, given that has_record has returned
    // true for _node.
    std::string get_type(ASTNode const& _node) const;

    // Returns the representative name for _node, given that has_record has
    // returned true for _node, whereas is_simple_type has filed.
    std::string get_name(ASTNode const& _node) const;

    // Returns true is _id is a pointer. If this cannot be resolved, false is
    // returned.
    bool is_pointer(Identifier const& _id) const;

    // Returns the simple type corresponding to _type.
    static std::string get_simple_ctype(Type const& _type);

    // Produces the initial value of a simple type.
    static CExprPtr init_val_by_simple_type(Type const& _type);

    // Generates the initial value for _typename.
	CExprPtr get_init_val(TypeName const& _typename) const;

    // Generates the initial value for _decl.
	CExprPtr get_init_val(Declaration const& _decl) const;

    // Provides a view of the map database.
    MapDeflate map_db() const;

protected:
    bool visit(VariableDeclaration const& _node) override;
	bool visit(ElementaryTypeName const& _node) override;
	bool visit(UserDefinedTypeName const& _node) override;
	bool visit(FunctionTypeName const&) override;
    bool visit(Mapping const& _node) override;
	bool visit(ArrayTypeName const&) override;
    bool visit(IndexAccess const& _node) override;
    bool visit(EventDefinition const&) override;

    void endVisit(ParameterList const& _node) override;
    void endVisit(MemberAccess const& _node) override;
	void endVisit(Identifier const& _node) override;

private:
    // m_global_context_types provides a basic mapping from name to type. If the
    // value is simple (has no "name"), it is in m_global_context_simple_values.
    static std::map<std::string, std::string> const m_global_context_types;
    static std::set<std::string> const m_global_simple_values;

    MapDeflate m_map_db;

    std::map<ASTNode const*, std::string> m_name_lookup;
    std::map<ASTNode const*, std::string> m_type_lookup;
    std::map<Identifier const*, bool> m_in_storage;

    bool m_is_retval = false;
};

// -------------------------------------------------------------------------- //

}
}
}
