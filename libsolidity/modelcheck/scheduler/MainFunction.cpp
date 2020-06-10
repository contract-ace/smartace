#include <libsolidity/modelcheck/scheduler/MainFunction.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/Mapping.h>
#include <libsolidity/modelcheck/utils/CallState.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>

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
    AllocationGraph const& _alloc_graph,
    CallState const& _statedata,
    TypeAnalyzer const& _converter
): M_STATEDATA(_statedata)
 , M_CONVERTER(_converter)
 , m_addrspace(_addrdata)
 , m_stategen(_statedata, _converter, _addrdata, _lockstep_time)
 , m_actors(_dependance, _converter, _alloc_graph, _addrdata)
{
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print(ostream& _stream)
{
    // Generates function switch.
    auto next_case = make_shared<CVarDecl>("uint8_t", "next_call");

    CBlockList default_case;
    LibVerify::require(default_case, Literals::ZERO);

    auto call_cases = make_shared<CSwitch>(next_case->id(), move(default_case));
    for (auto actor : m_actors.inspect())
    {
        for (auto const& spec : actor.specs)
        {
            auto call_body = build_case(spec, actor.decl);
            call_cases->add_case(call_cases->size(), move(call_body));
        }
    }

    // Contract setup and tear-down.
    CBlockList main;
    m_stategen.declare(main);
    m_actors.declare(main);
    m_addrspace.map_constants(main);
    m_actors.assign_addresses(main, m_addrspace);
    m_actors.initialize(main, M_STATEDATA, m_stategen);

    // Generates transactionals loop.
    CBlockList transactionals;
    transactionals.push_back(
        make_shared<CFuncCall>("sol_on_transaction", CArgList{})->stmt()
    );
    m_stategen.update(transactionals);
    transactionals.push_back(next_case);
    transactionals.push_back(next_case->assign(
        LibVerify::range(0, call_cases->size(), "next_call")
    )->stmt());
    transactionals.push_back(call_cases);

    // Adds transactional loop to end of body.
    LibVerify::log(main, "[Entering transaction loop]");
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
    FunctionSpecialization const& _spec, shared_ptr<CVarDecl const> _id
)
{
    CBlockList call_body;
    CExprPtr id = _id->id();
    if (!id->is_pointer())
    {
        id = make_shared<CReference>(id);
    }

    log_call(call_body, (*_id->id()), _spec);

    CFuncCallBuilder call_builder(_spec.name(0));
    call_builder.push(id);
    M_STATEDATA.push_state_to(call_builder);
    if (_spec.func().isPayable())
    {
        m_stategen.pay(call_body);
    }

    size_t placeholder_count = 0;
    for (auto const arg : _spec.func().parameters())
    {
        // Handles the case of unnamed (i.e., unused) inputs.
        string argname;
        CExprPtr value = nullptr;
        if (arg->name().empty())
        {
            argname = "placeholder_" + to_string(placeholder_count);
            placeholder_count += 1;
        }
        else
        {
            argname = "arg_" + arg->name();
            value = M_CONVERTER.get_nd_val(*arg, arg->name());
        }

        auto input = make_shared<CVarDecl>(
            M_CONVERTER.get_type(*arg), argname, false, value
        );

        call_body.push_back(input);
        call_builder.push(input->id());
    }

    call_body.push_back(call_builder.merge_and_pop_stmt());
    LibVerify::log(call_body, "[Call successful]");
    call_body.push_back(make_shared<CBreak>());

    return call_body;
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::log_call(
    CBlockList & _block,
    CIdentifier const& _id,
    FunctionSpecialization const& _call
)
{
    stringstream caselog;
    caselog << "[" << "Calling " << _call.func().name() << "(";

    auto const& PARAMS = _call.func().parameters();
    for (size_t i = 0; i < PARAMS.size(); ++i)
    {
        if (i != 0) caselog << ", ";

        auto const& NAME = PARAMS[i]->name();
        caselog << (NAME.empty() ? "0" : NAME);
    }

    caselog << ") on " << _id;
    caselog << "]";

    LibVerify::log(_block, caselog.str());
}

// -------------------------------------------------------------------------- //

}
}
}
