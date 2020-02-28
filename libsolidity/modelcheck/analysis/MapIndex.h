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
    enum class ViolationType { Cast, Mutate, Compare };

    // Describes a detected violation.
    struct Violation
    {
        ViolationType type;
        CallableDeclaration const* context;
        ASTNode const* site;
    };
    using ViolationGroup = std::list<Violation>;

    MapIndexSummary(uint64_t clients, uint64_t contracts);

    // Appends an analysis of _src. This will potentially add to
    // violations() and also literals().

    // A first-pass analysis which inspects contract code and extracts literals.
    void extract_literals(ContractDefinition const& _src);

    // A second-pass which computes the minimal interference needed.
    void compute_interference(ContractDefinition const& _src);

    // Returns all violates in the provided contract.
    ViolationGroup const& violations() const;

    // Returns all unique index literals encountered through the program.
    std::set<dev::u256> const& literals() const;

    // Returns the current number of representatives.
    uint64_t representative_count() const;

    // Returns the number of clients.
    uint64_t client_count() const;

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

    ViolationGroup m_violations;
    std::set<dev::u256> m_literals;
};

}
}
}
