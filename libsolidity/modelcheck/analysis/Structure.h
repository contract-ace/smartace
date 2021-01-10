/**
 * Analysis tools needed to summarize a structure.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/utils/Named.h>

#include <list>
#include <map>
#include <memory>
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
 * Provides a summary of a StructDefinition for SmartACE.
 */
class Structure : public Named
{
public:
    // Creates a summary of _structure.
    explicit Structure(StructDefinition const& _struct);

    // Returns the list of mappings required by the structure.
    std::list<Mapping const*> mappings() const;

    // Returns the fields of the structure.
    std::list<ASTPointer<VariableDeclaration>> fields() const;

    // TODO(scottwe): temporary solution to simplify transition.
    StructDefinition const* raw() const;

private:
    std::list<Mapping const*> m_mappings;
    std::list<ASTPointer<VariableDeclaration>> m_members;

    // TODO(scottwe): temporary solution to simplify transition.
    StructDefinition const* m_raw;
};

// -------------------------------------------------------------------------- //

class StructureContainer : public Named
{
public:
    // Creates a container which wraps the structures found on _contract.
    explicit StructureContainer(ContractDefinition const& _contract);

    virtual ~StructureContainer() = default;

    // Returns the structures defined by this contract.
    std::list<std::shared_ptr<Structure const>> structures() const;

    // Searches for a structure that mathces the declaration.
    std::shared_ptr<Structure const>
        find_structure(VariableDeclaration const* _decl) const;

    // TODO(scottwe): temporary solution to simplify transition.
    ContractDefinition const* raw() const;

private:
    std::list<std::shared_ptr<Structure const>> m_structures;

    std::map<StructDefinition const *, std::shared_ptr<Structure const>>
        m_structure_lookup;

    // TODO(scottwe): temporary solution to simplify transition.
    ContractDefinition const* m_raw;
};

// -------------------------------------------------------------------------- //

}
}
}
