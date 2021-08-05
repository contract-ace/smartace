/**
 * Analysis tools needed to summarize a structure.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/utils/Named.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Provides a summary of a StructDefinition for SmartACE.
 */
class Structure : public Named
{
public:
    // Creates a summary of _structure.
    explicit Structure(StructDefinition const& _struct);

    // Returns the mappings required by the structure.
    std::vector<Mapping const*> mappings() const;

    // Returns the fields of the structure.
    std::list<ASTPointer<VariableDeclaration>> fields() const;

    // TODO(scottwe): temporary solution to simplify transition.
    StructDefinition const* raw() const;

private:
    std::vector<Mapping const*> m_mappings;

    // The corresponding field defined by StructDefinition is a list.
    std::list<ASTPointer<VariableDeclaration>> m_members;

    // TODO(scottwe): temporary solution to simplify transition.
    StructDefinition const* m_raw;
};

// -------------------------------------------------------------------------- //

/**
 * An internal database, used to share structures between contracts and
 * libraries.
 */
class StructureStore
{
public:
    using Entry = std::shared_ptr<Structure const>;
    using Store = std::map<StructDefinition const *, Entry>;

    // Returns the SmartACE representation of _struct. If _struct has already
    // been queried, then a cached version is returned.
    Entry add(StructDefinition const *_struct);

    // Returns the SmartACE representation of _struct, if cached, else returns
    // a nullptr.
    Entry get(StructDefinition const *_struct);

    Store::const_iterator begin() const;
    Store::const_iterator end() const;

private:
    Store m_structure_lookup;
};

// -------------------------------------------------------------------------- //

/**
 * Interface to collection of structure declarations (i.e., contracts or
 * libraries).
 */
class StructureContainer : public Named
{
public:
    // Creates a container which wraps the structures found on _contract.
    explicit StructureContainer(
        ContractDefinition const& _contract,
        std::shared_ptr<StructureStore> _store
    );

    virtual ~StructureContainer() = default;

    // Returns the structures defined by this contract.
    std::vector<StructureStore::Entry> structures() const;

    // Searches for a structure that mathces the declaration.
    StructureStore::Entry find_structure(VariableDeclaration const* _decl) const;

    // TODO(scottwe): temporary solution to simplify transition.
    ContractDefinition const* raw() const;

private:
    std::vector<StructureStore::Entry> m_structures;

    // TODO(scottwe): temporary solution to simplify transition.
    ContractDefinition const* m_raw;

    // Helper method to extract structures from variable declarations.
    void record(VariableDeclaration const& _decl);

   std::shared_ptr<StructureStore> m_store;
};

// -------------------------------------------------------------------------- //

}
}
}
