/**
 * Provides interfaces to describe contract-based actors, and to maintain the
 * corresponding model in code.
 * @date 2020
 */

#include <libsolidity/modelcheck/harness/ActorModel.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
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
    CExprPtr _path,
    TicketSystem<uint16_t> & _cids,
    TicketSystem<uint16_t> & _fids
): contract(_contract)
 , path(_path)
{
    // Reserves a unique identifier for the actor.
    uint16_t cid = _cids.next();
    decl = make_shared<CVarDecl>(
        _converter.get_type(*_contract),
        "contract_" + to_string(cid),
        _path != nullptr
    );

    // Analyzes all children and function calls.
    for (auto method : _dependance.get_interface(_contract))
    {
        if (method->isConstructor()) continue;
        if (!method->isPublic()) continue;

        auto fnum = _fids.next();
        fnums[method] = fnum;
        specs.emplace_back(*method, *contract);

        for (size_t pidx = 0; pidx < method->parameters().size(); ++pidx)
        {
            auto PARAM = method->parameters()[pidx];

            ostringstream pname;
            pname << "c" << cid << "_f" << fnum << "_a" << pidx;
            if (!PARAM->name().empty()) pname << "_" << PARAM->name();

            fparams[PARAM.get()] = make_shared<CVarDecl>(
                _converter.get_type(*PARAM), pname.str()
            );
        }

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
    NewCallGraph const& _allocs,
    MapIndexSummary const& _addrdata
): m_converter(_converter)
 , m_addrdata(_addrdata)
{
    // Generates an actor for each client.
    TicketSystem<uint16_t> cids;
    TicketSystem<uint16_t> fids;
    for (auto const contract : _dependance.get_model())
    {
        if (contract->isLibrary() || contract->isInterface()) continue;

        m_actors.emplace_back(
            m_converter, _dependance, contract, nullptr, cids, fids
        );

        auto const& DECL = m_actors.back().decl;
        recursive_setup(
            _allocs, _dependance, DECL->id(), contract, cids, fids
        );
    }

    // Extracts the address variable for each contract.
    for (auto actor : m_actors)
    {
        for (auto entry : m_addrdata.describe(*actor.contract))
        {
            if (entry.paths.empty()) continue;

            if (entry.depth > 0)
            {
                throw runtime_error("Map to address unsupoorted.");
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
        for (auto param_pair : actor.fparams)
        {
            _block.push_back(param_pair.second);
        }
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
        caselog << "[Initializing " << (*actor.decl->id()) << "]";
        HarnessUtilities::log(_block, caselog.str());

        // Populates core constructor arguments.
        CFuncCallBuilder init_builder("Init_" + m_converter.get_name(*ctx));
        init_builder.push(make_shared<CReference>(actor.decl->id()));
        _statedata.push_state_to(init_builder);

        _stategen.update(_block, m_addrvar);

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
                init_builder.push(m_converter.get_nd_val(*param, MSG));
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
    CExprPtr _path,
    ContractDefinition const* _parent,
    TicketSystem<uint16_t> & _cids,
    TicketSystem<uint16_t> & _fids
)
{
    for (auto const& child : _allocs.children_of(_parent))
    {
        if (child.is_retval) continue;

        auto const NAME = VariableScopeResolver::rewrite(
            child.dest->name(), false, VarContext::STRUCT
        );
        auto const PATH = make_shared<CMemberAccess>(_path, NAME);

        m_actors.emplace_back(
            m_converter, _dependance, child.type, PATH, _cids, _fids
        );
        recursive_setup(_allocs, _dependance, PATH, child.type, _cids, _fids);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
