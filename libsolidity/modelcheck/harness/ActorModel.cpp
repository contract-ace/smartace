/**
 * Provides interfaces to describe contract-based actors, and to maintain the
 * corresponding model in code.
 * @date 2020
 */

#include <libsolidity/modelcheck/harness/ActorModel.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractDependance.h>
#include <libsolidity/modelcheck/analysis/MapIndex.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/harness/AddressSpace.h>
#include <libsolidity/modelcheck/harness/StateGenerator.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Harness.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

Actor::Actor(
    TypeConverter const& _converter,
    ContractDependance const& _dependance,
    ContractDefinition const* _contract,
    size_t _id,
    CExprPtr _path
): contract(_contract), path(_path)
{
    // Reserves a unique identifier for the actor.
    decl = make_shared<CVarDecl>(
        _converter.get_type(*_contract),
        "contract_" + to_string(_id),
        _path != nullptr
    );

    // Analyzes all children and function calls.
    for (auto method : _dependance.get_interface(_contract))
    {
        if (method->isConstructor()) continue;
        if (!method->isPublic()) continue;

        specs.emplace_back(*method, *contract);

        if (!_dependance.get_map_roi(method).empty())
        {
            uses_maps[method] = true;
        }
    }
}

// -------------------------------------------------------------------------- //

ActorModel::ActorModel(
    ContractDependance const& _dependance,
    TypeConverter const& _converter,
    NewCallGraph const& _newcalls,
    MapIndexSummary const& _addrdata
): M_CONVERTER(_converter)
{
    // Generates an actor for each client.
    for (auto const contract : _dependance.get_model())
    {
        if (contract->isLibrary() || contract->isInterface()) continue;

        m_actors.emplace_back(
            M_CONVERTER, _dependance, contract, m_actors.size(), nullptr
        );

        recursive_setup(_newcalls, _dependance, m_actors.back());
    }

    // Extracts the address variable for each contract.
    for (auto actor : m_actors)
    {
        for (auto entry : _addrdata.describe(*actor.contract))
        {
            if (entry.paths.empty()) continue;

            if (entry.depth > 0)
            {
                throw runtime_error("Map to address unsupported.");
            }
            
            for (auto path : entry.paths)
            {
                CExprPtr addr = actor.decl->id();
                for (auto symb : path)
                {
                    auto const NAME = VariableScopeResolver::rewrite(
                        symb, false, VarContext::STRUCT
                    );
                    addr = make_shared<CMemberAccess>(addr, NAME);
                }
                m_addrvar.push_back(make_shared<CMemberAccess>(addr, "v"));
            }
        }
    }
}

// -------------------------------------------------------------------------- //

void ActorModel::declare(CBlockList & _block) const
{
    // Declares each actor.
    for (auto const& actor : m_actors)
    {
        _block.push_back(actor.decl);
    }
}

// -------------------------------------------------------------------------- //

void ActorModel::initialize(
    CBlockList & _block,
    CallState const& _statedata,
    StateGenerator const& _stategen
) const
{
    // Initializes each actor.
    for (auto const& actor : m_actors)
    {
        if (actor.path) continue;

        auto ctx = actor.contract;

        stringstream caselog;
        caselog << "[Initializing " << (*actor.decl->id());
        if (actor.has_children) caselog << " and children";
        caselog << "]";
        HarnessUtilities::log(_block, caselog.str());

        // Populates core constructor arguments.
        auto init_builder = InitFunction(M_CONVERTER, *ctx).call_builder();
        init_builder.push(make_shared<CReference>(actor.decl->id()));
        _statedata.push_state_to(init_builder);

        _stategen.update(_block);

        // Populates specialized costructor arguments.
        if (auto const ctor = ctx->constructor())
        {
            if (ctor->isPayable())
            {
                _stategen.pay(_block);
            }

            for (auto const param : ctor->parameters())
            {
                string const MSG = ctx->name() + ":" + param->name();
                init_builder.push(M_CONVERTER.get_nd_val(*param, MSG));
            }
        }

        _block.push_back(init_builder.merge_and_pop()->stmt());
    }
}

// -------------------------------------------------------------------------- //

void ActorModel::assign_addresses(
    CBlockList & _block, AddressSpace & _addrspace
) const
{
    // Assigns an address to each contract
    for (auto const& actor : m_actors)
    {
        auto const& DECL = actor.decl;
        if (actor.path)
        {
            _block.push_back(
                DECL->assign(make_shared<CReference>(actor.path))->stmt()
            );
        }

        auto const& ADDR = DECL->access(ContractUtilities::address_member());
        _block.push_back(ADDR->access("v")->assign(
            make_shared<CIntLiteral>(_addrspace.reserve())
        )->stmt());
    }

    // Zero-initializes address variables.
    for (auto addr : m_addrvar)
    {
        _block.push_back(addr->assign(Literals::ZERO)->stmt());
    }
}

// -------------------------------------------------------------------------- //

list<shared_ptr<CMemberAccess>> const& ActorModel::vars() const
{
    // Returns all address declarations.
    return m_addrvar;
}

// -------------------------------------------------------------------------- //

list<Actor> const& ActorModel::inspect() const
{
    // Returns all actors.
    return m_actors;
}

// -------------------------------------------------------------------------- //

void ActorModel::recursive_setup(
    NewCallGraph const& _allocs,
    ContractDependance const& _dependance,
    Actor & _parent
)
{
    for (auto const& child : _allocs.children_of(_parent.contract))
    {
        if (child.is_retval) continue;
        _parent.has_children = true;

        auto const NAME = VariableScopeResolver::rewrite(
            child.dest->name(), false, VarContext::STRUCT
        );
        auto const PATH = make_shared<CMemberAccess>(_parent.decl->id(), NAME);

        m_actors.emplace_back(
            M_CONVERTER, _dependance, child.type, m_actors.size(), PATH
        );
        recursive_setup(_allocs, _dependance, m_actors.back());
    }
}

// -------------------------------------------------------------------------- //

}
}
}
