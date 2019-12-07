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

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MainFunctionGenerator::MainFunctionGenerator(
    size_t _keyspace,
    list<ContractDefinition const *> const& _model,
    NewCallGraph const& _new_graph,
    CallState const& _statedata,
    TypeConverter const& _converter
): M_KEYSPACE(_keyspace)
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

void MainFunctionGenerator::print(std::ostream& _stream)
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

    // Generates function switch.
    auto call_cases = make_shared<CSwitch>(
        get_nd_byte("Select next call"),
        CBlockList{make_require(Literals::ZERO)}
    );
    for (auto & actor : actors)
    {
        for (auto const& spec : actor.specs)
        {
            if (spec.func().isConstructor() || !spec.func().isPublic()) continue;
            auto call_body = build_case(spec, actor.fparams, actor.decl);
            call_cases->add_case(actor.fnums[&spec.func()], move(call_body));
        }
    }

    // Contract setup and tear-down.
    CBlockList main;

    // Declres all state variables.
    for (auto const& fld : m_statedata.order())
    {
        auto const DECL = make_shared<CVarDecl>(fld.tname, fld.name);
        main.push_back(DECL);

        if (fld.field == CallStateUtilities::Field::Block)
        {
            auto const TMP_DECL = make_shared<CVarDecl>(fld.tname, fld.temp);
            main.push_back(TMP_DECL);
            main.push_back(DECL->access("v")->assign(Literals::ZERO)->stmt());
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

    // Declares all addresses.
    auto const* ADDR_T = ContractUtilities::address_type();
    list<shared_ptr<CMemberAccess>> contract_addrs;
    vector<CExprPtr> addrs;
    addrs.reserve(M_KEYSPACE + actors.size());
    for (size_t i = 0; i < M_KEYSPACE; ++i)
    {
        auto const NAME = MapGenerator::name_global_key(i);
        auto const ADDRMSG = "Init address of " + NAME;
        auto decl = make_shared<CIdentifier>(NAME, false);
        main.push_back(decl->assign(
            TypeConverter::raw_simple_nd(*ADDR_T, ADDRMSG)
        )->stmt());
        addrs.push_back(decl);
    }
    for (auto const& actor : actors)
    {
        auto const& DECL = actor.decl;
        if (actor.path)
        {
            main.push_back(
                DECL->assign(make_shared<CReference>(actor.path))->stmt()
            );
        }

        auto const& ADDR = DECL->access(ContractUtilities::address_member());
        string const ADDRMSG = "Init address of " + actor.contract->name();
        main.push_back(ADDR->access("v")->assign(TypeConverter::raw_simple_nd(
            *ADDR_T, ADDRMSG
        ))->stmt());
        contract_addrs.push_back(ADDR);
        addrs.push_back(ADDR->access("v"));
    }
    for (unsigned int i = 0; i < addrs.size(); ++i)
    {
        main.push_back(make_require(make_shared<CBinaryOp>(
            addrs[i], "!=", Literals::ZERO
        )));
        for (unsigned int j = 0; j < i; ++j)
        {
            main.push_back(make_require(make_shared<CBinaryOp>(
                addrs[i], "!=", addrs[j]
            )));
        }
    }

    // Initializes all actors.
    for (auto const& actor : actors)
    {
        if (!actor.path)
        {
            update_call_state(main, contract_addrs);
            init_contract(main, *actor.contract, actor.decl);
        }
    }

    // Generates fixed-point iteration.
    CBlockList fixpoint;
    fixpoint.push_back(
        make_shared<CFuncCall>("sol_on_transaction", CArgList{})->stmt()
    );
    update_call_state(fixpoint, contract_addrs);
    fixpoint.push_back(call_cases);

    // Adds fixpoint look to end of body.
    main.push_back(make_shared<CWhileLoop>(
        make_shared<CBlock>(move(fixpoint)),
        get_nd_byte("Select 0 to terminate"),
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

    for (auto rel : contract->annotation().linearizedBaseContracts)
    {
        set<string> generated;
        for (auto const* FUNC : rel->definedFunctions())
        {
            if (FUNC->isConstructor() || !FUNC->isPublic()) continue;
            
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
        if (contract->isLibrary()) continue;

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
        auto const NAME = VariableScopeResolver::rewrite(
            child.dest->name(), false, VarContext::STRUCT
        );
        auto const PATH = make_shared<CMemberAccess>(_path, NAME);

        _actors.emplace_back(m_converter, child.type, PATH, _cids, _fids);
        analyze_nested_decls(_actors, PATH, child.type, _cids, _fids);
    }
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
    CBlockList & _stmts, list<shared_ptr<CMemberAccess>> const& _addresses
)
{
    for (auto const& fld : m_statedata.order())
    {
        auto state = make_shared<CIdentifier>(fld.name, false);
        auto nd = TypeConverter::raw_simple_nd(*fld.type, fld.name);

        if (fld.field == CallStateUtilities::Field::Paid) continue;

        if (fld.field == CallStateUtilities::Field::Block)
        {
            auto tmp_state = make_shared<CIdentifier>(fld.temp, false);
            _stmts.push_back(tmp_state->access("v")->assign(nd)->stmt());
            _stmts.push_back(make_require(make_shared<CBinaryOp>(
                state->access("v"), "<=", tmp_state->access("v")
            )));
            _stmts.push_back(state->assign(tmp_state)->stmt());
        }
        else if (fld.field == CallStateUtilities::Field::Value)
        {
            _stmts.push_back(state->access("v")->assign(Literals::ZERO)->stmt());
        }
        else
        {
            _stmts.push_back(state->access("v")->assign(nd)->stmt());
            if (fld.field == CallStateUtilities::Field::Sender)
            {
                _stmts.push_back(make_require(make_shared<CBinaryOp>(
                    state->access("v"), "!=", Literals::ZERO
                )));
                for (auto const addr : _addresses)
                {
                    _stmts.push_back(make_require(make_shared<CBinaryOp>(
                        state->access("v"), "!=", addr->access("v")
                    )));
                }
            }
        }
    }
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::set_payment_value(CBlockList & _stmts)
{
    auto const VAL_FIELD = CallStateUtilities::Field::Value;
    auto const VAL_NAME = CallStateUtilities::get_name(VAL_FIELD);
    auto const VAL_TYPE = CallStateUtilities::get_type(VAL_FIELD);
    auto nd = TypeConverter::raw_simple_nd(*VAL_TYPE, VAL_NAME);
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

}
}
}
