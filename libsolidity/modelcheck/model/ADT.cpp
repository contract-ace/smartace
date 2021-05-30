#include <libsolidity/modelcheck/model/ADT.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Library.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/model/Mapping.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/General.h>

#include <set>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ADTConverter::ADTConverter(
    shared_ptr<AnalysisStack const> _stack,
    bool _add_sums,
    size_t _map_k,
    bool _forward_declare
): M_ADD_SUMS(_add_sums)
 , M_MAP_K(_map_k)
 , M_FORWARD_DECLARE(_forward_declare)
 , m_stack(_stack)
{
}

void ADTConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);

    // Prints all libraries first.
    for (auto library : m_stack->libraries()->view())
    {
        for (auto structure : library->structures())
        {
            generate_structure(*structure);
        }
    }

    // And then prints all (required) contracts.
    for (auto contract : m_stack->model()->view())
    {
        generate_contract(*contract);
    }
}

// -------------------------------------------------------------------------- //

void ADTConverter::generate_contract(FlatContract const& _contract)
{
    if (!m_built.insert(&_contract).second) return;

    // Instantiated contracts must be encoded first.
    for (auto record : m_stack->model()->children_of(_contract))
    {
        generate_contract(*record.child);
    }

    // Next structures are encoded in the order they appear.
    for (auto structure : _contract.structures())
    {
        generate_structure(*structure);
    }

    // Next mappings are encoded, now that all structures are available.
    for (auto mapping : _contract.mappings())
    {
        generate_mapping(*mapping);
    }

    // Finally, the contract is encoded.
    shared_ptr<CParams> fields;
    if (!M_FORWARD_DECLARE)
    {
        fields = make_shared<CParams>();
        {
            TypePointer TYPE = ContractUtilities::address_type();
            string const TYPE_NAME = TypeAnalyzer::get_simple_ctype(*TYPE);
            string const NAME = ContractUtilities::address_member();

            fields->push_back(make_shared<CVarDecl>(TYPE_NAME, NAME));
        }
        {
            TypePointer TYPE = ContractUtilities::balance_type();
            string const TYPE_NAME = TypeAnalyzer::get_simple_ctype(*TYPE);
            string const NAME = ContractUtilities::balance_member();

            fields->push_back(make_shared<CVarDecl>(TYPE_NAME, NAME));
        }

        for (auto decl : _contract.state_variables())
        {
            if (decl->isConstant()) continue;

            // TODO: flat map to pre-compute the category.
            string type;
            auto const CATEGORY = decl->annotation().type->category();
            if (CATEGORY == Type::Category::Contract)
            {
                auto const& ref = m_stack->allocations()->specialize(*decl);
                type = m_stack->types()->get_type(ref);
            }
            else
            {
                type = m_stack->types()->get_type(*decl);
            }

            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            fields->push_back(make_shared<CVarDecl>(type, NAME));
        }
    }

    // TODO(scottwe): contracts should be able to name themselves.
    auto name = m_stack->types()->get_name(*_contract.raw());
    CStructDef contract(name, move(fields));
    (*m_ostream) << contract;
}

// -------------------------------------------------------------------------- //

void ADTConverter::generate_structure(Structure const& _structure)
{
    if (!m_built.insert(&_structure).second) return;

    // Prints the mapping dependencies.
    for (auto mapping : _structure.mappings())
    {
        generate_mapping(*mapping);
    }

    // Prints the structure.
    shared_ptr<CParams> fields;
    if (!M_FORWARD_DECLARE)
    {
        fields = make_shared<CParams>();
        for (auto decl : _structure.fields())
        {
            string const TYPE = m_stack->types()->get_type(*decl);
            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            fields->push_back(make_shared<CVarDecl>(TYPE, NAME));
        }
    }

    // TODO(scottwe): structures should be able to name themselves.
    auto name = m_stack->types()->get_name(*_structure.raw());
    CStructDef structure(name, move(fields));
    (*m_ostream) << structure;
}

// -------------------------------------------------------------------------- //

void ADTConverter::generate_mapping(Mapping const& _mapping)
{
    if (!m_built.insert(&_mapping).second) return;
    MapGenerator mapgen(_mapping, M_ADD_SUMS, M_MAP_K, *m_stack->types());
    (*m_ostream) << mapgen.declare(M_FORWARD_DECLARE);
}

// -------------------------------------------------------------------------- //

}
}
}
