/**
 * Provides the code required to flatten inheritance hierarchies.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/Structure.h>

#include <list>
#include <map>
#include <memory>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AllocationGraph;
class Structure;

// -------------------------------------------------------------------------- //

/**
 * A tree that describes the order of constructor calls, and sub-class
 * initialization. The tree corresponds to the linearized inheritance.
 */
class InheritanceTree
{
public:
    // A constructor invocation, and the corresponding arguments.
    struct InheritedCall
    {
        std::shared_ptr<InheritanceTree> parent;
        std::vector<ASTPointer<Expression>> args;
    };

    // Expands the constructor calls from _contract into a tee.
    InheritanceTree(ContractDefinition const& _contract);

    // Returns the constructor for this contract, if defined.
    FunctionDefinition const* constructor() const;

    // Returns all variables initialized at this level.
    std::vector<VariableDeclaration const*> const& decls() const;

    // Returns all ancestors initialized from this level, in order from least
    // derived to most derived.
    std::list<InheritedCall> const& baseContracts() const;

    // TODO: Deprecate.
    ContractDefinition const* raw() const;

    // If true, this contract cannot construct one of its base classes.
    bool is_abstract() const;

protected:
    // A record used to track the contracts initialized so far.
    struct LinearRecord
    {
        ContractDefinition const* contract;
        bool visited;
    };
    using LinearData = std::map<std::string, LinearRecord>;

    // Recursive ctor. to enable the sharing of _linear.
    InheritanceTree(LinearData & _linear, ContractDefinition const* _contract);

private:
    // Work-around since the public ctor. cannot delegate to the protected ctor.
    void initialize(LinearData & _linear, ContractDefinition const* _contract);

    // Constructs the ancestor specified by _decl, if not visited in _linear.
    void analyze_ancestor(
        LinearData & _linear,
        std::vector<ASTPointer<Expression>> const* _args,
        Declaration const& _decl
    );

    FunctionDefinition const* m_ctor;

    std::vector<VariableDeclaration const*> m_decls;

    std::list<InheritedCall> m_calls;

    ContractDefinition const* m_raw;

    bool m_abstract = false;
};

// -------------------------------------------------------------------------- //

/**
 * Creates a flat interface around the contract.
 */
class FlatContract : public StructureContainer
{
public:
    using FunctionList = std::list<FunctionDefinition const*>;
    using VariableList = std::list<VariableDeclaration const*>;

    // Flattens the inheritance tree of _contract.
    explicit FlatContract(ContractDefinition const& _contract);

    // Returns all exposed methods of the contract.
    FunctionList const& interface() const;

    // Returns all hidden methods of the contract.
    FunctionList const& internals() const;

    // Returns all inherited variables of the contract.
    VariableList const& state_variables() const;

    // Returns the constructor chain.
    FunctionList constructors() const;

    // Returns the fallback method.
    FunctionDefinition const* fallback() const;

    // Returns the inheritance tree.
    InheritanceTree const& tree() const;

    // Finds a method matching _func, or returns nullptr.
    FunctionDefinition const*
        try_resolve(FunctionDefinition const& _func) const;

    // Finds a method matching _func, or throws.
    FunctionDefinition const&
        resolve(FunctionDefinition const& _func) const;

    // Returns the mappings defiend (directly) by this contract.
    std::list<Mapping const*> mappings() const;

    // Returns true if the contract is payable.
    bool is_payable() const;

private:
    FunctionList m_public;
    FunctionList m_private;
    VariableList m_vars;

    FunctionList m_constructors;
    FunctionDefinition const* m_fallback = nullptr;

    InheritanceTree m_tree;

    std::list<Mapping const*> m_mappings;
};

// -------------------------------------------------------------------------- //

/**
 * An interface of flat contracts built for a specific model.
 */
class FlatModel
{
public:
    struct ChildRecord
    {
        std::shared_ptr<FlatContract> child;
        std::string var;
    };
    using ContractList = std::vector<ContractDefinition const *>;
    using FlatList = std::vector<std::shared_ptr<FlatContract>>;

    // This constructor has two conceptual steps. First, _model is expanded
    // using _allocation_graph. Afterwards, it is flattened to a set of
    // vertices, and each vertex is replaced by a FlatContract.
    FlatModel(
        ContractList const _model, AllocationGraph const& _allocation_graph
    );

    // Gives access to the bundle.
    FlatList bundle() const;

    // Gives view of all accessible contracts.
    FlatList view() const;

    // Retrieves the flattened _src. This also includes super interfaces.
    // Returns nullptr on failure.
    std::shared_ptr<FlatContract> get(ContractDefinition const& _src) const;

    // Retrieves the children of _contract.
    // TODO: This name is misleading. This is not inheritance.
    std::vector<ChildRecord> children_of(FlatContract const& _contract) const;

    // Retrieves the ancestor of _contract that precedes _ancestor .
    std::shared_ptr<FlatContract> next_ancestor(
        FlatContract const& _contract, FlatContract const& _ancestor
    ) const;

private:
    FlatList m_contracts;
    FlatList m_bundle;
    std::map<ContractDefinition const*, std::shared_ptr<FlatContract>> m_lookup;
    std::map<FlatContract const*, std::vector<ChildRecord>> m_children;
    std::map<FlatContract const*, FlatList> m_ancestors;
};

// -------------------------------------------------------------------------- //

}
}
}
