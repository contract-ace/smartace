/**
 * SmartACE computes PTGs to identify distinguishible addresses. Precisely,
 * each distinguishable address corresponds to an equivalence class of users.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <cstdint>
#include <list>
#include <memory>
#include <set>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallGraph;
class FlatContract;
class FlatModel;
class MapDeflate;

// -------------------------------------------------------------------------- //

/**
 * Records a MiniSol address violation.
 */
class AddressViolation
{
public:
    // Types of violations.
    // - Cast:      a (non-literal) numeric value was cast to an address value.
    // - Mutate:    an address value was mutated as if it were a numeric value.
    // - Compare:   two addresses were compared outside of (dis)equality.
    // - KeyType:   an unsupported key is used by a mapping.
    // - ValueType: an unsupported value is used by a mapping.
    enum class Type { Cast, Mutate, Compare, KeyType, ValueType };

    AddressViolation(
        Type _ty, CallableDeclaration const* _ctx, ASTNode const* _site
    ): m_type(_ty), m_context(_ctx), m_site(_site) {}

    // Returns the violation type.
    Type type() const { return m_type; }

    // Returns the call in which the violation occured. If the violation occurs
    // at the contract level, then context() is nullptr.
    CallableDeclaration const* context() const { return m_context; }

    // Returns the expression/statement resulting in the violation.
    ASTNode const* site() const { return m_site; }

protected:
    Type m_type;
    CallableDeclaration const* m_context;
    ASTNode const* m_site;
};

// -------------------------------------------------------------------------- //

/**
 * Extracts all literals in use by the bundle.
 */
class LiteralExtractor : public ASTConstVisitor
{
public:
    // Inspects all modifiers and functions within _calls, to extract all
    // literal values.
    LiteralExtractor(FlatModel const& _model, CallGraph const& _calls);

    // Returns all literals in use.
    std::set<dev::u256> literals() const;

    // Returns all vioaltes with respect to address manipulations.
    std::list<AddressViolation> violations() const;

protected:
    bool visit(UnaryOperation const& _node) override;
    bool visit(BinaryOperation const& _node) override;
    bool visit(FunctionCall const& _node) override;
    bool visit(MemberAccess const& _node) override;
    bool visit(Identifier const& _node) override;
    bool visit(Literal const& _node) override;

private:
    // Inspects FunctionCalls that are casts of `_base` from `_from` to `_to`.
    void handle_cast(
        Expression const& _base, Type::Category _from, Type::Category _to
    );

    // Inspects FunctionCalls that are not casts.
    void handle_call(FunctionCall const& _node);

    // Appennds AddressViolation(_ty, m_context, _site) to m_violations.
    void record_violation(AddressViolation::Type _ty, ASTNode const* _site);

    // Current function being analyzed.
    CallableDeclaration const* m_context = nullptr;

    // If true, then the current expression is nested within `address(...)`.
    bool m_is_address_cast = false;

    // The literal addresses detected so far.
    std::set<dev::u256> m_literals = { 0 };

    // Records all literal violations.
    std::list<AddressViolation> m_violations;
};

// -------------------------------------------------------------------------- //

/**
 * Computes the number of role used by a bundle. A role is an address state
 * variable. A role is active if it is used in the method.
 */
class RoleExtractor
{
public:
    using PathSet = std::list<std::list<std::string>>;

    // Represents all address paths associated with a given variable.
    struct Role
    {
        // The variable associated with this entry. It is a "root".
        VariableDeclaration const* decl;

        // If decl is a variable, this expands to decl. If dec is a structure
        // this is all paths to address variables in decl.
        PathSet paths;
    };

    // Computes the number of active roles in _contract.
    RoleExtractor(MapDeflate const& _map_db, FlatContract const& _contract);

    // Returns all roles in the contract, regardless of whether or not they are
    // in use.
    std::list<Role> roles() const;

    // Returns an over-approximation for the number of active roles.
    uint64_t count() const;

    // Returns all illegal uses of roles and mapping indices.
    std::list<AddressViolation> violations() const;

private:
    // Ensures that a mapping _decl does not make illegal usage of roles or
    // index typing.
    void check_map_conformance(VariableDeclaration const* _decl);

    // Computes all partial paths to roles within _struct.
    PathSet extract_from_struct(std::string _name, StructType const* _struct);

    // Appennds AddressViolation(_ty, m_context, _site) to m_violations.
    void record_violation(AddressViolation::Type _ty, ASTNode const* _site);

    MapDeflate const& m_map_db;

    // All roles in the contract, regardless of whether or not they are in use.
    std::list<Role> m_roles;

    // The number of active roles.
    uint64_t m_role_ct = 0;

    // Records all literal violations.
    std::list<AddressViolation> m_violations;
};

// -------------------------------------------------------------------------- //

/**
 * Computes the number of clients used by a bundle. A client is an address
 * variable (including msg.sender) that is passed to a method. A client is
 * active if it is used in the method.
 */
class ClientExtractor
{
public:
    // Comoutes the number of active clients in _model.
    ClientExtractor(FlatModel const& _model);

    // Returns an over-approximation for the number of active clients.
    uint64_t count() const;

private:
    // Utility to compute clients for a method.
    void compute_clients(FunctionDefinition const& _func);

    // The maximum number of clients in any function.
    uint64_t m_client_ct = 0;
};

// -------------------------------------------------------------------------- //

/**
 * An analyzer to determine the literal addresses, contract addresses, roles,
 * and clients. Conformance to MiniSol address usage is checked throughout the
 * analysis.
 */
class PTGBuilder
{
public:
    // Summarizes the PTGBuilder PTG for a MiniSol bundle with contracts in
    // _model. It is assumed that _calls is the call graph that corresponds to
    // _model. An additional _inf_ct clients are added to account for users
    // under interference and _aux clients are added to support certain classes
    // properties.
    PTGBuilder(
        MapDeflate const& _map_db,
        FlatModel const& _model,
        CallGraph const& _calls,
        bool _concrete,
        uint64_t _contract_ct,
        uint64_t _inf_ct,
        uint64_t _aux_ct
    );

    // Returns all unique index literals encountered through the program.
    std::set<dev::u256> const& literals() const;

    // Returns the roles for the given contract.
    std::list<RoleExtractor::Role>
    summarize(std::shared_ptr<FlatContract const> _contract) const;

    // Returns the number of contracts.
    uint64_t contract_count() const;

    // Returns the number of literals, contracts, and auxiliary users.
    uint64_t implicit_count() const;

    // Returns the number of roles and clients.
    uint64_t interference_count() const;

    // Returns the maximum address that can take part in a transaction (e.g., if
    // an invariant is being checked, then there must be one arbitrary user
    // under interference).
    uint64_t max_sender() const;

    // Returns the number of addresses.
    uint64_t count() const;

    // Returns all address violations.
    std::list<AddressViolation> violations() const;

private:
    bool m_concrete;

    uint64_t m_contract_ct = 0;
    uint64_t m_inf_ct = 0;
    uint64_t m_aux_ct = 0;
    uint64_t m_role_ct = 0;
    uint64_t m_client_ct = 0;

    // Records all literal violations.
    std::list<AddressViolation> m_violations;

    // Maps each contract to its role variables.
    std::map<FlatContract const*, std::list<RoleExtractor::Role>> m_role_lookup;

    // All literals in use.
    std::set<dev::u256> m_literals;
};

// -------------------------------------------------------------------------- //

}
}
}
