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

StructureContainer::StructureContainer(ContractDefinition const& _contract)
 : Named(_contract), m_raw(&_contract)
{
    auto const& contracts = _contract.annotation().linearizedBaseContracts;
    for (auto itr = contracts.crbegin(); itr != contracts.crend(); ++itr)
    {
        for (auto structure : (*itr)->definedStructs())
        {
            m_structures.push_back(make_shared<Structure>(*structure));
        }
    }
}

list<shared_ptr<Structure const>> StructureContainer::structures() const
{
    return m_structures;
}

ContractDefinition const* StructureContainer::raw() const { return m_raw; }

// -------------------------------------------------------------------------- //

}
}
}
