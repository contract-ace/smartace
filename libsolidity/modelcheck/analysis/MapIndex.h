/**
 * @date 2020
 * Provides utilities for generating an abstract address space for map indices.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <cstdint>
#include <list>
#include <set>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Maintains a database from contract to address variables.
 */
class AddressVariables
{
public:
    // Represents all address paths associated with a given variable.
    struct AddressEntry
    {
        // The variable associated with this entry. It is a "root".
        VariableDeclaration const* decl;

        // Either 0 and `decl` is not a mapping, or the depth of `decl`.
        uint64_t depth;

        // If false, this is a map and contains some invalid address.
        bool address_only;

        // If decl is a variable, this expands to decl. If dec is a structure
        // this is all paths to address variables in decl. If decl is a mapping,
        // paths will recursively be defined in terms of decl's values.
        std::list<std::list<std::string>> paths;
    };

    // Extracts all address paths within _src.
    void record(ContractDefinition const& _src);

    // Produces all address entries recorded against _src.
    std::list<AddressEntry> const& access(ContractDefinition const& _src) const;

private:
    // Helper utility to cast `T const*` to `R const*`.
    template <class R, class T>
    R const* unroll(T const* t)
    {
        return dynamic_cast<R const*>(t);
    }

    // Computes all paths to addresses within the struct.
    std::list<std::list<std::string>> analyze_struct(
        std::string _name, StructType const* _struct
    );

    // Computes the depth of the map, and if it maps to addresses, computes all
    // paths from a value.
    AddressEntry analyze_map(VariableDeclaration const& _decl);

    std::map<ContractDefinition const*, std::list<AddressEntry>> m_cache;
};

// -------------------------------------------------------------------------- //

/**
 * An analyzer to detect proper index usage. If index usage is conformed to,
 * then the contract-specific address-space parameters will be extracted.
 */
class MapIndexSummary : public ASTConstVisitor
{
public:
    // Types of violations.
    // - Cast:    an abstract index was cast to a concrete value
    // - Mutate:  an abstract index was mutated as a cardinal value
    // - Compare: an abstract index was ordered
    // - KeyType: an unsupported key type is in use
    enum class ViolationType { Cast, Mutate, Compare, KeyType };

    // Describes a detected violation.
    struct Violation
    {
        ViolationType type;
        CallableDeclaration const* context;
        ASTNode const* site;
    };
    using ViolationGroup = std::list<Violation>;

    // Generates map indices for use by the number of _clients and _contracts.
    MapIndexSummary(uint64_t _clients, uint64_t _contracts);
    
    // A first-pass analysis which inspects contract code and extracts literals.
    void extract_literals(ContractDefinition const& _src);

    // A second-pass which computes the minimal interference needed.
    void compute_interference(ContractDefinition const& _src);

    // Produces all address entries recorded against _src.
    std::list<AddressVariables::AddressEntry> const& describe(
        ContractDefinition const& _src
    ) const;

    // Returns all violates in the provided contract.
    ViolationGroup const& violations() const;

    // Returns all unique index literals encountered through the program.
    std::set<dev::u256> const& literals() const;

    // Returns the current number of representatives.
    uint64_t representative_count() const;

    // Returns the number of clients.
    uint64_t client_count() const;

    // Returns the number of contracts.
    uint64_t contract_count() const;

    // Returns the maximum interference variables used by any function.'
    uint64_t max_interference() const;

    // Returns the size of the address space.
    uint64_t size() const;

protected:
    bool visit(VariableDeclaration const& _node) override;
    bool visit(UnaryOperation const& _node) override;
    bool visit(BinaryOperation const& _node) override;
    bool visit(FunctionCall const& _node) override;
    bool visit(MemberAccess const& _node) override;
    bool visit(Identifier const& _node) override;
    bool visit(Literal const& _node) override;

private:
    uint64_t m_client_reps;
    uint64_t m_contract_reps;
    uint64_t m_max_inteference;

    bool m_in_first_pass;

    bool m_is_address_cast;
    bool m_uses_contract_address;
    
    CallableDeclaration const* m_context;

    AddressVariables m_cache;

    ViolationGroup m_violations;
    std::set<dev::u256> m_literals;
};

// -------------------------------------------------------------------------- //

}
}
}
