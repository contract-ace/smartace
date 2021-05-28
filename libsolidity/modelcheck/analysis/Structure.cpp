#include <libsolidity/modelcheck/analysis/Structure.h>

#include <libsolidity/modelcheck/analysis/Mapping.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

Structure::Structure(StructDefinition const& _struct)
 : Named(_struct)
 , m_mappings(MappingExtractor(_struct.members()).get())
 , m_members(_struct.members().begin(), _struct.members().end())
 , m_raw(&_struct)
{}

list<ASTPointer<VariableDeclaration>> Structure::fields() const
{
    return m_members;
}

list<Mapping const*> Structure::mappings() const { return m_mappings; }

StructDefinition const* Structure::raw() const { return m_raw; }

// -------------------------------------------------------------------------- //

shared_ptr<Structure const> StructureStore::get(StructDefinition const *_struct)
{
    auto result = m_structure_lookup.find(_struct);
    if (result != m_structure_lookup.end())
    {
        return result->second;
    }
    else
    {
        auto record = make_shared<Structure>(*_struct);
        m_structure_lookup[_struct] = record;
        return record;
    }
}

// -------------------------------------------------------------------------- //

StructureContainer::StructureContainer(
    ContractDefinition const& _contract, StructureStore & _store
): Named(_contract), m_raw(&_contract)
{
    auto const& contracts = _contract.annotation().linearizedBaseContracts;
    for (auto itr = contracts.crbegin(); itr != contracts.crend(); ++itr)
    {
        // Records declarations.
        for (auto structure : (*itr)->definedStructs())
        {
            m_structures.push_back(_store.get(structure));
            m_structure_lookup[structure] = m_structures.back();
        }

        // Records library uses.
        for (auto decl : _contract.stateVariables())
        {
            record(*decl);
        }
    }
}

list<shared_ptr<Structure const>> StructureContainer::structures() const
{
    return m_structures;
}

shared_ptr<Structure const>
    StructureContainer::find_structure(VariableDeclaration const* _decl) const
{
    if (auto type = dynamic_cast<StructType const*>(_decl->type()))
    {
        auto result = m_structure_lookup.find(&type->structDefinition());
        if (result != m_structure_lookup.end())
        {
            return result->second;
        }
    }
    return nullptr;
}

ContractDefinition const* StructureContainer::raw() const { return m_raw; }

void StructureContainer::record(VariableDeclaration const& _decl)
{
    
    if (auto type = dynamic_cast<StructType const*>(_decl.type()))
    {
        auto const& STRUCT = type->structDefinition();
        auto result = m_structure_lookup.find(&STRUCT);
        if (result == m_structure_lookup.end())
        {
            m_structure_lookup[&STRUCT] = make_shared<Structure>(STRUCT);
            for (auto decl : STRUCT.members())
            {
                record(*decl);
            }
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
