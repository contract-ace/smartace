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

// -------------------------------------------------------------------------- //

/**
 * Maintains a dictionary from AST Node addresses to type encoding metadata. The
 * mapping is restricted to AST Nodes for which types are practical, and records
 * must be generated on a per-source-unit basis.
 */
class TypeAnalyzer : public ASTConstVisitor
{
public:
    // Creates a type analyzer with _addrs addresses. Zero addresses is taken to
    // denote unbounded addresses.
    TypeAnalyzer(uint64_t _addrs = 0);

    // Generates type encoding metadata for all relevant members of _unit. The
    // type annotations may be recovered using the translate method.
    void record(SourceUnit const& _unit);

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
    CExprPtr raw_simple_nd(Type const& _type, std::string const& _msg) const;
    CExprPtr nd_val_by_simple_type(
        Type const& _type, std::string const& _msg
    ) const;

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

    // Provides a view of the map database.
    MapDeflate map_db() const;

protected:
    bool visit(VariableDeclaration const& _node) override;
	bool visit(ElementaryTypeName const& _node) override;
	bool visit(UserDefinedTypeName const& _node) override;
	bool visit(FunctionTypeName const& _node) override;
    bool visit(Mapping const& _node) override;
	bool visit(ArrayTypeName const& _node) override;
    bool visit(IndexAccess const& _node) override;
    bool visit(EmitStatement const&) override;
    bool visit(EventDefinition const&) override;

    void endVisit(ParameterList const& _node) override;
    void endVisit(MemberAccess const& _node) override;
	void endVisit(Identifier const& _node) override;

private:
    // m_global_context_types provides a basic mapping from name to type. If the
    // value is simple (has no "name"), it is in m_global_context_simple_values.
    static std::map<std::string, std::string> const m_global_context_types;
    static std::set<std::string> const m_global_context_simple_values;

    uint64_t m_address_count = 0;

    MapDeflate m_map_db;

    std::map<ASTNode const*, std::string> m_name_lookup;
    std::map<ASTNode const*, std::string> m_type_lookup;
    std::map<Identifier const*, bool> m_in_storage;

    ContractDefinition const* m_curr_contract = nullptr;
    VariableDeclaration const* m_curr_decl = nullptr;
    bool m_is_retval = false;
};

// -------------------------------------------------------------------------- //

}
}
}
