/**
 * Provides the code required to flatten inheritance hierarchies.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

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

// -------------------------------------------------------------------------- //

/**
 * Creates a flat interface around the contract.
 */
class FlatContract
{
public:
    using FunctionList = std::list<FunctionDefinition const*>;
    using VariableList = std::list<VariableDeclaration const*>;

    // Flattens the inheritance tree of _contract.
    FlatContract(ContractDefinition const& _contract);

    // Returns all exposed methods of the contract.
    FunctionList const& interface() const;

    // Returns all inherited variables of the contract.
    VariableList const& state_variables() const;

    // Returns the name of the contract.
    std::string const& name() const;

    // Returns the constructor chain.
    FunctionList constructors() const;

    // Returns the fallback method.
    FunctionDefinition const* fallback() const;

    // Finds a method matching _func, or throws.
    FunctionDefinition const& resolve(FunctionDefinition const& _func) const;

private:
    std::string const M_NAME;

    FunctionList m_public;
    FunctionList m_private;
    VariableList m_vars;

    FunctionList m_constructors;
    FunctionDefinition const* m_fallback;
};

// -------------------------------------------------------------------------- //

/**
 * An interface of flat contracts built for a specific model.
 */
class FlatModel
{
public:
    using ContractList = std::vector<ContractDefinition const *>;
    using FlatList = std::vector<std::shared_ptr<FlatContract>>;

    // This constructor has two conceptual steps. First, _model is expanded
    // using _alloc_graph. Afterwards, it is flattened to a set of vertices, and
    // each vertex is replaced by a FlatContract.
    FlatModel(ContractList _model, AllocationGraph const& _alloc_graph);

    // Gives view of all contracts.
    FlatList const& view() const;

    // Retrieves the flattened _src. This also includes super interfaces.
    // Returns nullptr on failure.
    std::shared_ptr<FlatContract> get(ContractDefinition const& _src) const;

private:
    FlatList m_contracts;
    std::map<ContractDefinition const*, std::shared_ptr<FlatContract>> m_lookup;
};

// -------------------------------------------------------------------------- //

}
}
}
