#include <libsolidity/modelcheck/scheduler/MainFunction.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
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
    CompInvarGenerator::Settings _settings,
    shared_ptr<AnalysisStack const> _stack,
    shared_ptr<NondetSourceRegistry> _nd_reg
): m_stack(_stack)
 , m_nd_reg(_nd_reg)
 , m_addrspace(_stack->addresses(), _nd_reg)
 , m_stategen(_stack, _nd_reg, _lockstep_time)
 , m_actors(_stack, _nd_reg)
 , m_invars(_stack, m_actors, _settings)
{
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print_invariants(ostream& _stream)
{
    m_invars.print_invariants(_stream);
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print_globals(ostream& _stream)
{
    m_actors.declare_global(_stream);
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print_main(ostream& _stream)
{
    // Generates function switch.
    auto next_case = make_shared<CVarDecl>("uint8_t", "next_call");

    CBlockList default_case;
    string default_err("Model failure, next_call out of bounds.");
    LibVerify::add_require(default_case, Literals::ZERO, default_err);

    size_t case_count = 0;
    auto call_cases = make_shared<CSwitch>(next_case->id(), move(default_case));
    for (auto actor : m_actors.inspect())
    {
        for (auto const& spec : actor.specs)
        {
            auto call_body = build_case(spec, actor.decl);
            call_cases->add_case(call_cases->size(), move(call_body));
            case_count += 1;
        }
    }

    if (case_count == 0)
    {
        throw runtime_error("Bundle has no public or external calls.");
    }

    // Contract setup and tear-down.
    CBlockList main;
    m_stategen.declare(main);
    m_actors.declare(main);
    m_addrspace.map_constants(main);
    m_actors.assign_addresses(main);
    m_actors.initialize(main, m_stategen);

    // Generates transactionals loop.
    CBlockList transactionals;
    transactionals.push_back(
        make_shared<CFuncCall>("sol_on_transaction", CArgList{})->stmt()
    );
    transactionals.push_back(make_shared<CIf>(
        make_shared<CFuncCall>("sol_can_infer", CArgList{}),
        make_shared<CBlock>(m_invars.check_interference(*m_nd_reg))
    ));
    transactionals.push_back(make_shared<CIf>(
        make_shared<CFuncCall>("sol_can_infer", CArgList{}),
        make_shared<CBlock>(m_invars.apply_interference(*m_nd_reg))
    ));
    m_stategen.update_global(transactionals);
    transactionals.push_back(next_case);
    transactionals.push_back(next_case->assign(
        m_nd_reg->range(0, call_cases->size(), "next_call")
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

    m_stategen.update_local(call_body);

    CFuncCallBuilder call_builder(_spec.name(0));
    call_builder.push(id);
    m_stack->environment()->push_state_to(call_builder);
    if (_spec.func().isPayable())
    {
        m_stategen.pay(call_body);
    }

    for (size_t i = 1; i < _spec.func().returnParameters().size(); ++i)
    {
        auto const& rv = _spec.func().returnParameters()[i];
        string name = "rv_" + to_string(i);
        string type = m_stack->types()->get_type(*rv.get());

        auto output = make_shared<CVarDecl>(type, name);
        call_body.push_back(output);
        call_builder.push(make_shared<CReference>(output->id()));
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
            value = m_nd_reg->val(*arg, arg->name());
        }

        auto input = make_shared<CVarDecl>(
            m_stack->types()->get_type(*arg), argname, false, value
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
