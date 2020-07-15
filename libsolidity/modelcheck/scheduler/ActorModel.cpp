#include <libsolidity/modelcheck/scheduler/ActorModel.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/scheduler/AddressSpace.h>
#include <libsolidity/modelcheck/scheduler/StateGenerator.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

Actor::Actor(
    shared_ptr<AnalysisStack const> _stack,
    std::shared_ptr<FlatContract const> _contract,
    size_t _id,
    CExprPtr _path
): contract(_contract), path(_path)
{
    // Reserves a unique identifier for the actor.
    decl = make_shared<CVarDecl>(
        _stack->types()->get_type(*_contract->raw()),
        "contract_" + to_string(_id),
        _path != nullptr
    );

    // Analyzes all children and function calls.
    for (auto method : _contract->interface())
    {
        specs.emplace_back(*method, *contract->raw());
    }
}

// -------------------------------------------------------------------------- //

ActorModel::ActorModel(shared_ptr<AnalysisStack const> _stack)
 : m_stack(_stack)
{
    // Generates an actor for each client.
    for (auto const contract : m_stack->model()->bundle())
    {
        m_actors.emplace_back(m_stack, contract, m_actors.size(), nullptr);
        recursive_setup(m_actors.back());
    }

    // Extracts the address variable for each contract.
    for (auto actor : m_actors)
    {
        for (auto entry : m_stack->addresses()->describe(*actor.contract->raw()))
        {
            if (entry.paths.empty()) continue;

            if (entry.depth > 0)
            {
                throw runtime_error("Map to address unsupported.");
            }
            
            for (auto path : entry.paths)
            {
                CExprPtr addr = actor.decl->id();
                for (auto id : path)
                {
                    auto const NAME = VariableScopeResolver::rewrite(
                        id, false, VarContext::STRUCT
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
    CBlockList & _block, StateGenerator const& _stategen
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
        LibVerify::log(_block, caselog.str());

        // Populates core constructor arguments.
        auto init = InitFunction(*m_stack->types(), *ctx->raw()).call_builder();
        init.push(make_shared<CReference>(actor.decl->id()));
        m_stack->environment()->push_state_to(init);

        _stategen.update_global(_block);

        // Populates specialized costructor arguments.
        CBlockList init_block;
        _stategen.update_local(init_block);
        if (!ctx->constructors().empty())
        {
            if (auto const ctor = ctx->constructors().front())
            {
                if (ctor->isPayable())
                {
                    _stategen.pay(init_block);
                }

                for (auto const param : ctor->parameters())
                {
                    string const MSG = ctx->name() + ":" + param->name();
                    init.push(m_stack->types()->get_nd_val(*param, MSG));
                }
            }
        }
        init_block.push_back(init.merge_and_pop()->stmt());
        _block.push_back(make_shared<CBlock>(move(init_block)));
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

void ActorModel::recursive_setup(Actor & _parent)
{
    for (auto const& record : m_stack->model()->children_of(*_parent.contract))
    {
        _parent.has_children = true;

        auto const NAME = VariableScopeResolver::rewrite(
            record.var, false, VarContext::STRUCT
        );
        auto const PATH = make_shared<CMemberAccess>(_parent.decl->id(), NAME);

        m_actors.emplace_back(m_stack, record.child, m_actors.size(), PATH);
        recursive_setup(m_actors.back());
    }
}

// -------------------------------------------------------------------------- //

}
}
}
