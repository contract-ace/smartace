
#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/utils/Function.h>

#include <set>
#include <stdexcept>
#include <string>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

InheritanceTree::InheritanceTree(ContractDefinition const& _contract)
{
    // Prepares map.
    LinearData linear;
    for (auto relative : _contract.annotation().linearizedBaseContracts)
    {
        linear[relative->name()]
            = LinearRecord{ relative, (relative == &_contract) };
    }
    m_calls.reserve(linear.size());

    // Calls initializer.
    initialize(linear, &_contract);
}

FunctionDefinition const* InheritanceTree::constructor() const
{
    return m_ctor;
}

vector<VariableDeclaration const*> const& InheritanceTree::decls() const
{
    return m_decls;
}

vector<InheritanceTree::InheritedCall> const&
InheritanceTree::baseContracts() const
{
    return m_calls;
}

ContractDefinition const* InheritanceTree::raw() const
{
    return m_raw;
}

InheritanceTree::InheritanceTree(
    LinearData & _linear, ContractDefinition const* _contract
)
{
    initialize(_linear, _contract);
}

void InheritanceTree::initialize(
    LinearData & _linear, ContractDefinition const* _contract
)
{
    // Sets variables.
    m_raw = _contract;
    m_ctor = _contract->constructor();
    m_decls = _contract->stateVariables();

    // Handles constructor calls (represented as modifiers).
    // This happens if there is a constructor, and at least one base class.
    if (_linear.size() > 1 && m_ctor)
    {
        auto const& MODS = m_ctor->modifiers();
        if (!m_ctor->modifiers().empty())
        {
            // Modifier lookup.
            for (auto mod : MODS)
            {
                auto REF = mod->name()->annotation().referencedDeclaration;
                analyze_ancestor(_linear, mod->arguments(), *REF);
            }
        }
    }

    // Handles direct constructor calls.
    for (auto parent : _contract->baseContracts())
    {
        auto REF = parent->name().annotation().referencedDeclaration;
        analyze_ancestor(_linear, parent->arguments(), *REF);
    }
}

void InheritanceTree::analyze_ancestor(
    LinearData & _linear,
    vector<ASTPointer<Expression>> const* _args,
    Declaration const& _decl
)
{
    auto itr = _linear.find(_decl.name());

    // Ensure contract can be initialized.
    if (itr->second.contract->isInterface()) return;
    if (itr->second.visited) return;

    // Checks if arguments are passed to contract.
    if (!_args)
    {
        if (auto ctor = itr->second.contract->constructor())
        {
            if (!ctor->parameters().empty())
            {
                m_abstract = true;
                return;
            }
        }
    }

    // Records visit.
    itr->second.visited = true;

    m_calls.emplace_back();

    if (_args) m_calls.back().args = (*_args);

    shared_ptr<InheritanceTree>
        parent(new InheritanceTree(_linear, itr->second.contract));
    m_calls.back().parent = move(parent);
}

bool InheritanceTree::is_abstract() const
{
    return m_abstract;
}

// -------------------------------------------------------------------------- //

FlatContract::FlatContract(
    ContractDefinition const& _contract, shared_ptr<StructureStore> _store
): StructureContainer(_contract, _store), m_tree(_contract)
{
    MappingExtractor extractor;

    map<string, FunctionList> registered_functions;
    set<string> variable_names;
    set<string> modifier_names;
    for (auto c : _contract.annotation().linearizedBaseContracts)
    {
        // If this is an interface, there is nothing to do.
        if (c->isInterface()) continue;

        // Checks for functions with new signatures.
        m_public.reserve(m_public.size() + c->definedFunctions().size());
        m_private.reserve(m_private.size() + c->definedFunctions().size());
        for (auto f : c->definedFunctions())
        {
            // Searchs for a fallback.
            if (!m_fallback && f->isFallback())
            {
                m_fallback = f;
                continue;
            }

            // Accumulates constructors.
            if (f->isConstructor())
            {
                m_constructors.push_back(f);
                continue;
            }

            // Checks for duplicates.
            auto & entries = registered_functions[f->name()];
            bool found_match = false;
            for (auto candidate : entries)
            {
                found_match = collid(*f, *candidate);
                if (found_match) break;
            }
            if (found_match) continue;

            // It is new, so register it.
            entries.push_back(f);
            if (f->functionType(false))
            {
                m_public.push_back(f);
            }
            else
            {
                m_private.push_back(f);
            }
        }

        // Checks for new modifiers.
        m_modifiers.reserve(m_modifiers.size() + c->functionModifiers().size());
        for (auto m : c->functionModifiers())
        {
            if (modifier_names.insert(m->name()).second)
            {
                m_modifiers.push_back(m);
            }
        }

        // Checks for new variables.
        m_vars.reserve(m_vars.size() + c->stateVariables().size());
        for (auto v : c->stateVariables())
        {
            if (variable_names.insert(v->name()).second)
            {
                m_vars.push_back(v);
                extractor.record(v);
            }
            else if (v->visibility() == Declaration::Visibility::Private ||
                     v->visibility() == Declaration::Visibility::Default)
            {
                string const MSG1 = "Two or more private variables named ";
                string const MSG2 = " in contract ";
                throw runtime_error(MSG1 + v->name() + MSG2 + name());
            }
        }

        // Records enum definitions.
        m_enums.reserve(m_enums.size() + c->definedEnums().size());
        for (auto e : c->definedEnums())
        {
            m_enums.push_back(e);
        }
    }

    // Generates map list.
    m_mappings = extractor.get();
}

FlatContract::FunctionList const& FlatContract::interface() const
{
    return m_public;
}

FlatContract::FunctionList const& FlatContract::internals() const
{
    return m_private;
}

FlatContract::VariableList const& FlatContract::state_variables() const
{
    return m_vars;
}

FlatContract::FunctionList const& FlatContract::constructors() const
{
    return m_constructors;
}

FunctionDefinition const* FlatContract::fallback() const
{
    return m_fallback;
}

FlatContract::ModifierList const& FlatContract::modifiers() const
{
    return m_modifiers;
}

InheritanceTree const& FlatContract::tree() const
{
    return m_tree;
}

FunctionDefinition const*
    FlatContract::try_resolve(FunctionDefinition const& _func) const
{
    if (_func.isFallback())
    {
        if (m_fallback)
        {
            return m_fallback;
        }
    }
    else if (_func.functionType(false))
    {
        for (auto method : m_public)
        {
            if (collid(_func, *method)) return method;
        }
    }
    else
    {
        for (auto method : m_private)
        {
            if (collid(_func, *method)) return method;
        }
    }

    return nullptr;
}

FunctionDefinition const&
    FlatContract::resolve(FunctionDefinition const& _func) const
{
    if (auto res = try_resolve(_func))
    {
        return (*res);
    }
    else
    {
        string const FN = _func.name();
        string const MSG1 = "Could not resolve function (";
        string const MSG2 = ") against flat contract (";
        string const MSG3 = ").";
        throw runtime_error(MSG1 + FN + MSG2 + name() + MSG3);
    }
}

vector<Mapping const*> const& FlatContract::mappings() const
{
    return m_mappings;
}

vector<EnumDefinition const*> const& FlatContract::enums() const
{
    return m_enums;
}

bool FlatContract::is_payable() const
{
    return (m_fallback && m_fallback->isPayable());
}

// -------------------------------------------------------------------------- //

FlatModel::FlatModel(
    FlatModel::ContractList const _model,
    AllocationGraph const& _allocation_graph,
    shared_ptr<StructureStore> _store
)
{
    // Iterates through all children.
    set<ContractDefinition const*> visited;
    FlatModel::ContractList pending = _model;
    for (size_t i = 0; i < pending.size(); ++i)
    {
        // Checks if this is a new contract.
        auto contract = pending[i];
        if (!visited.insert(contract).second) continue;

        // Records the contract.
        m_contracts.push_back(make_shared<FlatContract>(*contract, _store));
        m_lookup[contract] = m_contracts.back();

        // Adds children to the list.
        for (auto child : _allocation_graph.children_of(contract))
        {
            pending.push_back(child.type);
        }
    }

    // Performs a second pass to add parents and children.
    auto processed = visited;
    for (auto raw_contract : processed)
    {
        auto contract = m_lookup[raw_contract];

        // Initializes all parents.
        auto const& LBC = raw_contract->annotation().linearizedBaseContracts;
        auto & ancestors = m_ancestors[contract.get()];
        ancestors.reserve(LBC.size());
        for (auto raw_parent : LBC)
        {
            shared_ptr<FlatContract> parent;
            if (visited.insert(raw_parent).second)
            {
                parent = make_shared<FlatContract>(*raw_parent, _store);
                m_lookup[raw_parent] = parent;
            }
            else
            {
                parent = m_lookup[raw_parent];
            }
            ancestors.push_back(parent);
        }

        // Associates all parents to their children.
        for (auto child : _allocation_graph.children_of(raw_contract))
        {
            ChildRecord record{m_lookup[child.type], child.dest->name()};
            m_children[contract.get()].push_back(std::move(record));
        }
    }

    // Records the deployed contracts in the model.
    for (auto contract : _model)
    {
        auto flat = m_lookup[contract];
        if (flat->tree().is_abstract()) continue;
        m_bundle.push_back(flat);
    }
}

FlatModel::FlatList FlatModel::bundle() const { return m_bundle; }

FlatModel::FlatList FlatModel::view() const { return m_contracts; }

shared_ptr<FlatContract> FlatModel::get(ContractDefinition const& _src) const
{
    auto match = m_lookup.find(&_src);
    if (match != m_lookup.end())
    {
        return match->second;
    }
    return nullptr;
}

std::vector<FlatModel::ChildRecord>
    FlatModel::children_of(FlatContract const& _contract) const
{
    auto match = m_children.find(&_contract);
    if (match != m_children.end())
    {
        return match->second;
    }
    return {};
}

shared_ptr<FlatContract> FlatModel::next_ancestor(
    FlatContract const& _contract, FlatContract const& _ancestor
) const
{
    auto match = m_ancestors.find(&_contract);
    if (match != m_ancestors.end())
    {
        for (size_t i = 0; i < match->second.size() - 1; ++i)
        {
            if (match->second[i].get() == (&_ancestor))
            {
                return match->second[i + 1];
            }
        }
    }
    return nullptr;
}

// -------------------------------------------------------------------------- //

}
}
}
