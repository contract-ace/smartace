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

vector<Mapping const*> Structure::mappings() const { return m_mappings; }

StructDefinition const* Structure::raw() const { return m_raw; }

// -------------------------------------------------------------------------- //

StructureStore::Entry StructureStore::add(StructDefinition const *_struct)
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

StructureStore::Entry StructureStore::get(StructDefinition const *_struct)
{
    auto result = m_structure_lookup.find(_struct);
    if (result != m_structure_lookup.end())
    {
        return result->second;
    }
    else
    {
        return nullptr;
    }
}

StructureStore::Store::const_iterator StructureStore::begin() const
{
    return m_structure_lookup.begin();
}

StructureStore::Store::const_iterator StructureStore::end() const
{
    return m_structure_lookup.end();
}

// -------------------------------------------------------------------------- //

StructureContainer::StructureContainer(
    ContractDefinition const& _contract, shared_ptr<StructureStore> _store
): Named(_contract), m_raw(&_contract), m_store(_store)
{
    auto const& contracts = _contract.annotation().linearizedBaseContracts;
    for (auto itr = contracts.crbegin(); itr != contracts.crend(); ++itr)
    {
        // Records declarations.
        for (auto structure : (*itr)->definedStructs())
        {
            m_structures.push_back(m_store->add(structure));
        }

        // Records library uses.
        for (auto decl : _contract.stateVariables())
        {
            record(*decl);
        }
    }
}

vector<StructureStore::Entry> StructureContainer::structures() const
{
    return m_structures;
}

StructureStore::Entry
    StructureContainer::find_structure(VariableDeclaration const* _decl) const
{
    if (auto type = dynamic_cast<StructType const*>(_decl->type()))
    {
        auto const& STRUCT = type->structDefinition();
        return m_store->get(&STRUCT);
    }
    return nullptr;
}

ContractDefinition const* StructureContainer::raw() const { return m_raw; }

void StructureContainer::record(VariableDeclaration const& _decl)
{
    if (auto type = dynamic_cast<StructType const*>(_decl.type()))
    {
        // Records structure.
        auto const& STRUCT = type->structDefinition();
        m_store->add(&STRUCT);

        // Continues search.
        for (auto decl : STRUCT.members())
        {
            record(*decl);
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
