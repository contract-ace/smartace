/*
 * @date 2019
 * This model maps each Solidity type to a C-type. For structures and contracts,
 * these are synthesized C-structs. This translation unit provides utilities for
 * performing such conversions.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
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

/*
 * Helper method to calculate the depth of an array index access.
 */
class AccessDepthResolver : public ASTConstVisitor
{
public:
    // Sets up a resolver for a mapping index access at AST node _base.
    AccessDepthResolver(IndexAccess const& _base);

    // Returns the Mapping TypeName used in the declaration of _base's mapping
    // expression. If this cannot be resolved, null is returned.
    Mapping const* resolve();

protected:
	bool visit(Conditional const&) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(IndexAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
	Expression const& m_base;

	unsigned int m_submap_count;
    VariableDeclaration const* m_decl;
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

    // Returns true if this node was found in the AST, and that it has been
    // recorded with a type. If it is not a simple type, then it should also
    // have a name.
    bool has_record(ASTNode const& _node) const;

    // Returns the CType used to model _node, given that has_record has returned
    // true for _node.
    std::string get_type(ASTNode const& _node) const;

    // Returns the representative name for _node, given that has_record has
    // returned true for _node, whereas is_simple_type has filed.
    std::string get_name(ASTNode const& _node) const;

    // Returns true is an identifier is a pointer. If this cannot be resolved,
    // false is returned.
    bool is_pointer(Identifier const& _id) const;

    // Returns the simple type corresponding to _type.
    static std::string get_simple_ctype(Type const& _type);

    // Produces the initial value of a simple type.
    static CExprPtr init_val_by_simple_type(Type const& _type);

    // Produces a non-deterministic value for a simple type.
    static CExprPtr raw_simple_nd(Type const& _type, std::string const& _msg);
    static CExprPtr nd_val_by_simple_type(
        Type const& _type, std::string const& _msg
    );

    // Generates the initial value for an AST node.
	CExprPtr get_init_val(TypeName const& _typename) const;
	CExprPtr get_init_val(Declaration const& _decl) const;

    // Generates a non-deterministic value for an AST node.
	CExprPtr get_nd_val(
        TypeName const& _typename, std::string const& _msg
    ) const;
	CExprPtr get_nd_val(
        Declaration const& _decl, std::string const& _msg
    ) const;

protected:
    bool visit(VariableDeclaration const& _node) override;
	bool visit(ElementaryTypeName const& _node) override;
	bool visit(UserDefinedTypeName const& _node) override;
	bool visit(FunctionTypeName const& _node) override;
    bool visit(Mapping const&) override;
	bool visit(ArrayTypeName const& _node) override;
    bool visit(EmitStatement const&) override;
    bool visit(EventDefinition const&) override;

    void endVisit(ParameterList const& _node) override;
	void endVisit(Mapping const& _node) override;
    void endVisit(MemberAccess const& _node) override;
    void endVisit(IndexAccess const& _node) override;
	void endVisit(Identifier const& _node) override;

private:
    // m_global_context_types provides a basic mapping from name to type. If the
    // value is simple (has no "name"), it is in m_global_context_simple_values.
    static std::map<std::string, std::string> const m_global_context_types;
    static std::set<std::string> const m_global_context_simple_values;

    std::map<ASTNode const*, std::string> m_name_lookup;
    std::map<ASTNode const*, std::string> m_type_lookup;
    std::map<Identifier const*, bool> m_in_storage;

    ContractDefinition const* m_curr_contract = nullptr;
    VariableDeclaration const* m_curr_decl = nullptr;
    unsigned int m_rectype_depth = 0;
    bool m_is_retval = false;
};

}
}
}
