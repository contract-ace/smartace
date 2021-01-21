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
    //
    struct InheritedCall
    {
        std::shared_ptr<InheritanceTree> parent;
        std::vector<ASTPointer<Expression>> args;
    };

    //
    InheritanceTree(ContractDefinition const& _contract);

    //
    FunctionDefinition const* constructor() const;

    //
    std::vector<VariableDeclaration const*> const& decls() const;

    //
    std::list<InheritedCall> const& baseContracts() const;

    //
    ContractDefinition const* raw() const;

    //
    bool is_abstract() const;

protected:
    // 
    struct LinearRecord
    {
        ContractDefinition const* contract;
        bool visited;
    };
    using LinearData = std::map<std::string, LinearRecord>;

    //
    InheritanceTree(LinearData & _linear, ContractDefinition const* _contract);

private:
    //
    void initialize(LinearData & _linear, ContractDefinition const* _contract);

    //
    void analyze_ancestor(
        LinearData & _linear,
        std::vector<ASTPointer<Expression>> const* _args,
        Declaration const& _decl
    );

    //
    FunctionDefinition const* m_ctor;

    //
    std::vector<VariableDeclaration const*> m_decls;

    //
    std::list<InheritedCall> m_calls;

    //
    ContractDefinition const* m_raw;

    //
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

    // Retrieves the children of the contract.
    std::vector<ChildRecord> children_of(FlatContract const& _contract) const;

private:
    FlatList m_contracts;
    FlatList m_bundle;
    std::map<ContractDefinition const*, std::shared_ptr<FlatContract>> m_lookup;
    std::map<FlatContract const*, std::vector<ChildRecord>> m_children;
};

// -------------------------------------------------------------------------- //

}
}
}
