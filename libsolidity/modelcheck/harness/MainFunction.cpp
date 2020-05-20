/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
 */

#include <libsolidity/modelcheck/harness/MainFunction.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/MapIndex.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/Mapping.h>
#include <libsolidity/modelcheck/utils/CallState.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Harness.h>
#include <libsolidity/modelcheck/utils/Indices.h>

#include <set>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MainFunctionGenerator::MainFunctionGenerator(
    bool _lockstep_time,
    MapIndexSummary const& _addrdata,
        ContractDependance const& _dependance,
    NewCallGraph const& _new_graph,
    CallState const& _statedata,
    TypeConverter const& _converter
): m_addrspace(_addrdata)
 , m_stategen(_statedata, _converter, _addrdata, _lockstep_time)
 , m_actors(_dependance, _converter, _new_graph, _addrdata)
 , m_addrdata(_addrdata)
 , m_statedata(_statedata)
 , m_converter(_converter)
{
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print(ostream& _stream)
{
    // Generates function switch.
    size_t case_count = 0;
    auto next_case = make_shared<CVarDecl>("uint8_t", "next_call");

    CBlockList default_case;
    HarnessUtilities::require(default_case, Literals::ZERO);

    auto call_cases = make_shared<CSwitch>(next_case->id(), move(default_case));
    for (auto actor : m_actors.inspect())
    {
        for (auto const& spec : actor.specs)
        {
            bool uses_maps = actor.uses_maps[&spec.func()];
            auto call_body = build_case(
                spec, actor.fparams, actor.decl, uses_maps
            );
            call_cases->add_case(actor.fnums[&spec.func()], move(call_body));
            ++case_count;
        }
    }

    // Contract setup and tear-down.
    CBlockList main;
    m_stategen.declare(main);
    m_actors.declare(main);
    m_addrspace.map_constants(main);
    m_actors.assign_addresses(main, m_addrspace);
    m_actors.initialize(main, m_statedata, m_stategen);

    // Generates transactionals loop.
    CBlockList transactionals;
    transactionals.push_back(
        make_shared<CFuncCall>("sol_on_transaction", CArgList{})->stmt()
    );
    m_stategen.update(transactionals, m_actors.vars());
    transactionals.push_back(next_case);
    transactionals.push_back(next_case->assign(
        HarnessUtilities::range(0, case_count, "next_call")
    )->stmt());
    transactionals.push_back(call_cases);

    // Adds transactional loop to end of body.
    HarnessUtilities::log(main, "[Entering transaction loop]");
    main.push_back(make_shared<CWhileLoop>(
        make_shared<CBlock>(move(transactionals)),
        make_shared<CFuncCall>("sol_continue", CArgList{}),
        false
    ));

    // Implements body as a run_model function.
    auto id = make_shared<CVarDecl>("void", "run_model");
    _stream << CFuncDef(id, CParams{}, make_shared<CBlock>(move(main)));
}

// -------------------------------------------------------------------------- //

CBlockList MainFunctionGenerator::build_case(
    FunctionSpecialization const& _spec,
    map<VariableDeclaration const*, shared_ptr<CVarDecl>> & _args,
    shared_ptr<const CVarDecl> _id,
    bool uses_maps
)
{
    CExprPtr id = _id->id();
    if (!id->is_pointer())
    {
        id = make_shared<CReference>(id);
    }

    CBlockList call_body;

    stringstream caselog;
    caselog << "[Calling " << _spec.func().name()
            << " on " << (*_id->id()) << "]";
    HarnessUtilities::log(call_body, caselog.str());

    // TODO: actually populate maps.
    if (uses_maps)
    {
        call_body.push_back(make_shared<CComment>("Uses maps."));
    }

    CFuncCallBuilder call_builder(_spec.name());
    call_builder.push(id);
    m_statedata.push_state_to(call_builder);

    if (_spec.func().isPayable())
    {
        m_stategen.pay(call_body);
    }
    for (auto const arg : _spec.func().parameters())
    {
        string const MSG = _spec.func().name() + ":" + arg->name();
        auto const& PDECL = _args[arg.get()];
        auto const ND_VAL = m_converter.get_nd_val(*arg, MSG);
        call_body.push_back(PDECL->assign(ND_VAL)->stmt());
        call_builder.push(PDECL->id());
    }
    call_body.push_back(call_builder.merge_and_pop_stmt());
    HarnessUtilities::log(call_body, "[Call successful]");
    call_body.push_back(make_shared<CBreak>());

    return call_body;
}

// -------------------------------------------------------------------------- //

}
}
}
