/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
 */

#include <libsolidity/modelcheck/translation/MainFunction.h>

#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/translation/Mapping.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Indices.h>

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
    list<ContractDefinition const *> const& _model,
    NewCallGraph const& _new_graph,
    CallState const& _statedata,
    TypeConverter const& _converter
): M_LOCKSTEP_TIME(_lockstep_time)
 , M_USES_ZERO(_addrdata.literals().find(0) != _addrdata.literals().end())
 , m_addrdata(_addrdata)
 , m_model(_model)
 , m_new_graph(_new_graph)
 , m_statedata(_statedata)
 , m_converter(_converter)
{
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::record(SourceUnit const& _ast)
{
    auto inset = ASTNode::filteredNodes<ContractDefinition>(_ast.nodes());
    m_contracts.insert(m_contracts.end(), inset.begin(), inset.end());
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print(ostream& _stream)
{
    // Determines the contracts to use by the harness.
    vector<ContractDefinition const*> model;
    if (!m_model.empty())
    {
        model.reserve(m_model.size());
        model.insert(model.begin(), m_model.begin(), m_model.end());
    }
    else
    {
        model.reserve(m_contracts.size());
        model.insert(model.begin(), m_contracts.begin(), m_contracts.end());
    }

    // Pre-analyzes contracts for fields, etc.
    list<Actor> actors = analyze_decls(model);
    list<CExprPtr> addr_vars;
    for (auto actor : actors)
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
                addr_vars.push_back(make_shared<CMemberAccess>(addr, "v"));
            }
        }
    }

    // Generates function switch.
    size_t case_count = 0;
    auto next_case = make_shared<CVarDecl>("uint8_t", "next_call");
    auto call_cases = make_shared<CSwitch>(
        next_case->id(), CBlockList{make_require(Literals::ZERO)}
    );
    for (auto & actor : actors)
    {
        for (auto const& spec : actor.specs)
        {
            auto call_body = build_case(spec, actor.fparams, actor.decl);
            call_cases->add_case(actor.fnums[&spec.func()], move(call_body));
            ++case_count;
        }
    }

    // Contract setup and tear-down.
    CBlockList main;

    // Declres all state variables.
    auto timestep_var = make_shared<CVarDecl>("uint8_t", "take_step");
    main.push_back(timestep_var);
    for (auto const& fld : m_statedata.order())
    {
        auto const DECL = make_shared<CVarDecl>(fld.tname, fld.name);
        main.push_back(DECL);

        if (fld.field == CallStateUtilities::Field::Block ||
            fld.field == CallStateUtilities::Field::Timestamp)
        {
            auto const TMP_DECL = make_shared<CVarDecl>(fld.tname, fld.temp);
            main.push_back(TMP_DECL);

            if (M_LOCKSTEP_TIME)
            {
                auto nd = m_converter.raw_simple_nd(*fld.type, fld.name);
                main.push_back(
                    DECL->access("v")->assign(nd)->stmt()
                );
            }
            else
            {
                main.push_back(
                    DECL->access("v")->assign(Literals::ZERO)->stmt()
                );
            }
        }
        else if (fld.field == CallStateUtilities::Field::Paid)
        {
            main.push_back(DECL->access("v")->assign(Literals::ONE)->stmt());
        }
    }

    // Declares function parameters.
    for (auto const& actor : actors)
    {
        main.push_back(actor.decl);
        for (auto param_pair : actor.fparams) main.push_back(param_pair.second);
    }

    // Assigns all addresses.
    init_address_space(main, actors);

    // Initializes all actors.
    for (auto const& actor : actors)
    {
        if (!actor.path)
        {
            update_call_state(main, addr_vars);
            init_contract(main, *actor.contract, actor.decl);
        }
    }

    // Generates transactionals loop.
    CBlockList transactionals;
    transactionals.push_back(
        make_shared<CFuncCall>("sol_on_transaction", CArgList{})->stmt()
    );
    update_call_state(transactionals, addr_vars);
    transactionals.push_back(next_case);
    transactionals.push_back(
        next_case->assign(get_nd_range(0, case_count, "next_call"))->stmt()
    );
    transactionals.push_back(call_cases);

    // Adds transactional loop to end of body.
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

CStmtPtr MainFunctionGenerator::make_require(CExprPtr _cond)
{
    auto fn = make_shared<CFuncCall>(
        "sol_require", CArgList{_cond, Literals::ZERO
    });
    return fn->stmt();
}

// -------------------------------------------------------------------------- //

MainFunctionGenerator::Actor::Actor(
    TypeConverter const& _converter,
    ContractDefinition const* _contract,
    CExprPtr _path,
    TicketSystem<uint16_t> & _cids,
    TicketSystem<uint16_t> & _fids
): contract(_contract), path(_path)
{
    uint16_t cid = _cids.next();

    decl = make_shared<CVarDecl>(
        _converter.get_type(*_contract),
        "contract_" + to_string(cid),
        _path != nullptr
    );

    set<string> generated;
    for (auto rel : contract->annotation().linearizedBaseContracts)
    {
        for (auto const* FUNC : rel->definedFunctions())
        {
            if (FUNC->isConstructor()) continue;
            if (!FUNC->isPublic()) continue;
            if (!FUNC->isImplemented()) continue;
            
            auto res = generated.insert(FUNC->name());
            if (!res.second) continue;

            auto fnum = _fids.next();
            fnums[FUNC] = fnum;
            specs.emplace_back(*FUNC, *contract);

            for (size_t pidx = 0; pidx < FUNC->parameters().size(); ++pidx)
            {
                auto PARAM = FUNC->parameters()[pidx];

                ostringstream pname;
                pname << "c" << cid << "_f" << fnum << "_a" << pidx;
                if (!PARAM->name().empty()) pname << "_" << PARAM->name();

                fparams[PARAM.get()] = make_shared<CVarDecl>(
                    _converter.get_type(*PARAM), pname.str()
                );
            }
        }
    }
}

// -------------------------------------------------------------------------- //

list<MainFunctionGenerator::Actor> MainFunctionGenerator::analyze_decls(
    vector<ContractDefinition const*> const& _contracts
) const
{
    list<Actor> actors;
    TicketSystem<uint16_t> cids;
    TicketSystem<uint16_t> fids;

    for (auto const contract : _contracts)
    {
        if (contract->isLibrary() || contract->isInterface()) continue;

        actors.emplace_back(m_converter, contract, nullptr, cids, fids);

        auto const& DECL = actors.back().decl;
        analyze_nested_decls(actors, DECL->id(), contract, cids, fids);
    }

    return actors;
}

void MainFunctionGenerator::analyze_nested_decls(
    list<Actor> & _actors,
    CExprPtr _path,
    ContractDefinition const* _parent,
    TicketSystem<uint16_t> & _cids,
    TicketSystem<uint16_t> & _fids
) const
{
    auto const& children = m_new_graph.children_of(_parent);

    for (auto const& child : children)
    {
        if (child.is_retval) continue;

        auto const NAME = VariableScopeResolver::rewrite(
            child.dest->name(), false, VarContext::STRUCT
        );
        auto const PATH = make_shared<CMemberAccess>(_path, NAME);

        _actors.emplace_back(m_converter, child.type, PATH, _cids, _fids);
        analyze_nested_decls(_actors, PATH, child.type, _cids, _fids);
    }
}

// -------------------------------------------------------------------------- //

list<shared_ptr<CMemberAccess>> MainFunctionGenerator::init_address_space(
    CBlockList & _stmts, std::list<Actor> const& _actors
)
{
    // If the null address is in use, it is address 0.
    uint64_t min = (M_USES_ZERO ? 1 : 0);

    // Maps all contracts into the address space.
    list<shared_ptr<CMemberAccess>> contract_addrs;
    for (auto const& actor : _actors)
    {
        auto const& DECL = actor.decl;
        if (actor.path)
        {
            _stmts.push_back(
                DECL->assign(make_shared<CReference>(actor.path))->stmt()
            );
        }

        auto const& ADDR = DECL->access(ContractUtilities::address_member());
        _stmts.push_back(ADDR->access("v")->assign(
            make_shared<CIntLiteral>(min + contract_addrs.size())
        )->stmt());
        contract_addrs.push_back(ADDR);
    }

    // Maps all literals into the address space.
    list<shared_ptr<CIdentifier>> used_so_far;
    for (auto lit : m_addrdata.literals())
    {
        auto const NAME = modelcheck::Indices::const_global_name(lit);
        auto decl = make_shared<CIdentifier>(NAME, false);

        if (lit == 0)
        {
            _stmts.push_back(decl->assign(
                make_shared<CIntLiteral>(0)
            )->stmt());
        }
        else
        {
            _stmts.push_back(decl->assign(get_nd_range(
                min, m_addrdata.representative_count(), NAME
            ))->stmt());

            for (auto otr : used_so_far)
            {
                // TOD: bad for fuzzing, though used_so_far is often small.
                _stmts.push_back(make_shared<CBinaryOp>(
                    decl, "!=", otr
                )->stmt());
            }

            used_so_far.push_back(decl);
        }
    }

    return contract_addrs;
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::init_contract(
    CBlockList & _stmts,
    ContractDefinition const& _contract,
    shared_ptr<const CVarDecl> _id
)
{
    CFuncCallBuilder init_builder("Init_" + m_converter.get_name(_contract));
    init_builder.push(make_shared<CReference>(_id->id()));
    m_statedata.push_state_to(init_builder);

    if (auto const ctor = _contract.constructor())
    {
        if (ctor->isPayable())
        {
            set_payment_value(_stmts);
        }

        for (auto const param : ctor->parameters())
        {
            string const MSG = _contract.name() + ":" + param->name();
            init_builder.push(m_converter.get_nd_val(*param, MSG));
        }
    }

    _stmts.push_back(init_builder.merge_and_pop()->stmt());
}

// -------------------------------------------------------------------------- //

CBlockList MainFunctionGenerator::build_case(
    FunctionSpecialization const& _spec,
    map<VariableDeclaration const*, shared_ptr<CVarDecl>> & _args,
    shared_ptr<const CVarDecl> _id
)
{
    CExprPtr id = _id->id();
    if (!id->is_pointer())
    {
        id = make_shared<CReference>(id);
    }

    CFuncCallBuilder call_builder(_spec.name());
    call_builder.push(id);
    m_statedata.push_state_to(call_builder);

    CBlockList call_body;
    if (_spec.func().isPayable())
    {
        set_payment_value(call_body);
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
    call_body.push_back(make_shared<CBreak>());

    return call_body;
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::update_call_state(
    CBlockList & _stmts, list<CExprPtr> const& _addrvars
)
{
    // Decides once, if lockstep will be used.
    auto timestep_var = make_shared<CIdentifier>("take_step", false);
    if (M_LOCKSTEP_TIME)
    {
        _stmts.push_back(
            timestep_var->assign(get_nd_range(0, 2, "take_step"))->stmt()
        );
    }

    // Shuffles address variables which point to interference. The shuffle is
    // performed with respect to the first address, so it is skipped.
    {
        uint64_t minaddr = m_addrdata.representative_count();
        uint64_t maxaddr = m_addrdata.size();
        auto boundary = make_shared<CIntLiteral>(minaddr);
        for (auto itr = (++_addrvars.begin()); itr != _addrvars.end(); ++itr)
        {
            // TODO: better message.
            auto value_range = get_nd_range(minaddr, maxaddr, "addrvar");

            auto var = (*itr);
            auto update = make_shared<CBinaryOp>(var, "=", value_range)->stmt();
            auto cond = make_shared<CBinaryOp>(var, ">=", boundary);
            _stmts.push_back(make_shared<CIf>(cond, update, nullptr));
        }
    }

    // Updates the values.
    for (auto const& fld : m_statedata.order())
    {
        auto state = make_shared<CIdentifier>(fld.name, false);
        auto nd = m_converter.raw_simple_nd(*fld.type, fld.name);

        if (fld.field == CallStateUtilities::Field::Paid) continue;
        if (fld.field == CallStateUtilities::Field::Origin) continue;

        if (fld.field == CallStateUtilities::Field::Block ||
            fld.field == CallStateUtilities::Field::Timestamp)
        {
            auto tmp_state = make_shared<CIdentifier>(fld.temp, false);
            if (M_LOCKSTEP_TIME)
            {
                CBlockList step;
                step.push_back(tmp_state->access("v")->assign(nd)->stmt());
                step.push_back(make_require(make_shared<CBinaryOp>(
                    state->access("v"), "<", tmp_state->access("v")
                )));

                _stmts.push_back(make_shared<CIf>(
                    timestep_var, make_shared<CBlock>(move(step)), nullptr
                ));
            }
            else
            {
                // TOD: it would be ideal to drop the <=.
                _stmts.push_back(tmp_state->access("v")->assign(nd)->stmt());
                _stmts.push_back(make_require(make_shared<CBinaryOp>(
                    state->access("v"), "<=", tmp_state->access("v")
                )));
            }
            _stmts.push_back(state->assign(tmp_state)->stmt());
        }
        else if (fld.field == CallStateUtilities::Field::Value)
        {
            _stmts.push_back(state->access("v")->assign(Literals::ZERO)->stmt());
        }
        else if (fld.field == CallStateUtilities::Field::Sender)
        {
            // This restricts senders to valid addresses: non-zero clients.
            size_t minaddr = m_addrdata.contract_count();
            size_t maxaddr = m_addrdata.size();
            if (M_USES_ZERO)
            {
                minaddr += 1;
            }

            auto ndaddr = get_nd_range(minaddr, maxaddr, fld.name);
            _stmts.push_back(state->access("v")->assign(ndaddr)->stmt());
        }
        else
        {
            _stmts.push_back(state->access("v")->assign(nd)->stmt());
        }
    }
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::set_payment_value(CBlockList & _stmts)
{
    auto const VAL_FIELD = CallStateUtilities::Field::Value;
    auto const VAL_NAME = CallStateUtilities::get_name(VAL_FIELD);
    auto const VAL_TYPE = CallStateUtilities::get_type(VAL_FIELD);
    auto nd = m_converter.raw_simple_nd(*VAL_TYPE, VAL_NAME);
    auto state = make_shared<CIdentifier>(VAL_NAME, false);
    _stmts.push_back(state->access("v")->assign(nd)->stmt());
}

// -------------------------------------------------------------------------- //

CExprPtr MainFunctionGenerator::get_nd_byte(string const& _msg)
{
    return make_shared<CFuncCall>(
        "rt_nd_byte", CArgList{make_shared<CStringLiteral>(_msg)}
    );
}

// -------------------------------------------------------------------------- //

CExprPtr MainFunctionGenerator::get_nd_range(
    uint8_t _l, uint8_t _u, string const& _msg
)
{
    return make_shared<CFuncCall>(
        "rt_nd_range", CArgList{
            make_shared<CIntLiteral>(_l),
            make_shared<CIntLiteral>(_u),
            make_shared<CStringLiteral>(_msg)
        }
    );
}

// -------------------------------------------------------------------------- //

}
}
}
