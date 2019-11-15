/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
 */

#include <libsolidity/modelcheck/MainFunctionGenerator.h>

#include <libsolidity/modelcheck/Contract.h>
#include <libsolidity/modelcheck/Function.h>
#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/TranslationLiterals.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MainFunctionGenerator::MainFunctionGenerator(
    SourceUnit const& _ast, TypeConverter const& _converter
): m_ast(_ast), m_converter(_converter)
{
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print(std::ostream& _stream)
{
    auto contracts = ASTNode::filteredNodes<ContractDefinition>(m_ast.nodes());
    if (contracts.size() > 1)
    {
        throw runtime_error("Multiple contracts not yet supported.");
    }

    auto const CURSTATE = make_shared<CVarDecl>("struct CallState", "curstate");
    auto const NXTSTATE = make_shared<CVarDecl>("struct CallState", "nxtstate");

    // Pre-analyzes contracts for fields, etc.
    map<VariableDeclaration const*, shared_ptr<CVarDecl>> param_decls;
    map<ContractDefinition const*, shared_ptr<CVarDecl>> contract_decls;
    map<FunctionDefinition const*, uint64_t> func_id;
    analyze_decls(contracts, param_decls, contract_decls, func_id);

    // Generates function switch.
    auto call_cases = make_shared<CSwitch>(
        get_nd_byte("Select next call"),
        CBlockList{make_require(Literals::ZERO)}
    );
    for (auto const* CONTRACT : contracts)
    {
        auto CDECL = contract_decls[CONTRACT];
        for (auto const* FUNC : CONTRACT->definedFunctions())
        {
            if (FUNC->isConstructor() || !FUNC->isPublic()) continue;
            auto call_body = build_case(*FUNC, param_decls, CDECL, CURSTATE);
            call_cases->add_case(func_id[FUNC], move(call_body));
        }
    }

    // Generates fixed-point iteration.
    CBlockList fixpoint;
    fixpoint.push_back(
        make_shared<CFuncCall>("sol_on_transaction", CArgList{})->stmt()
    );
    update_call_state(fixpoint, NXTSTATE);
    fixpoint.push_back(make_require(make_shared<CBinaryOp>(
        make_shared<CMemberAccess>(NXTSTATE->access("blocknum"), "v"),
        ">=",
        make_shared<CMemberAccess>(CURSTATE->access("blocknum"), "v")
    )));
    fixpoint.push_back(CURSTATE->assign(NXTSTATE->id())->stmt());
    fixpoint.push_back(call_cases);

    // Contract setup and tear-down.
    CBlockList main;
    for (auto param_pair : param_decls) main.push_back(param_pair.second);
    main.push_back(CURSTATE);
    update_call_state(main, CURSTATE);
    main.push_back(NXTSTATE);
    for (auto contract_pair : contract_decls)
    {
        auto const& DECL = contract_pair.second;
        auto const& ADDR = DECL->access(ContractUtilities::address_member());
        string const ADDRMSG = "Init address of " + contract_pair.first->name();
        main.push_back(DECL);
        main.push_back(init_contract(*contract_pair.first, DECL, CURSTATE));
        main.push_back(ADDR->assign(
            TypeConverter::nd_val_by_simple_type(
                *ContractUtilities::address_type(), ADDRMSG
            )
        )->stmt());
        main.push_back(make_require(make_shared<CBinaryOp>(
            make_shared<CMemberAccess>(ADDR, "v"), "!=", Literals::ZERO
        )));
    }
    main.push_back(make_shared<CWhileLoop>(
        make_shared<CBlock>(move(fixpoint)),
        get_nd_byte("Select 0 to terminate"),
        false
    ));

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

void MainFunctionGenerator::analyze_decls(
    vector<ContractDefinition const*> const& _contracts, 
    map<VariableDeclaration const*, shared_ptr<CVarDecl>> & _dcls,
    map<ContractDefinition const*, shared_ptr<CVarDecl>> & _defs,
    map<FunctionDefinition const*, uint64_t> & _funcs
)
{
    uint32_t fid = 0;
    for (unsigned int i = 0; i < _contracts.size(); ++i)
    {
        auto const* CONTRACT = _contracts[i];
        string const TYPE = m_converter.get_type(*CONTRACT);
        _defs[CONTRACT] = make_shared<CVarDecl>(TYPE, "contract");

        for (unsigned int j = 0; j < CONTRACT->definedFunctions().size(); ++j)
        {
            auto const* FUNC = CONTRACT->definedFunctions()[j];
            if (FUNC->isConstructor() || !FUNC->isPublic()) continue;
            for (unsigned int k = 0; k < FUNC->parameters().size(); ++k)
            {
                ASTPointer<const VariableDeclaration> PARAM
                    = FUNC->parameters()[k];

                string const TYPE = m_converter.get_type(*PARAM);
                ostringstream param_name;
                param_name << "c" << i << "_f" << j << "_a" << k;
                if (!PARAM->name().empty()) param_name << "_" << PARAM->name();

                auto param_decl = make_shared<CVarDecl>(TYPE, param_name.str());
                _dcls[PARAM.get()] = param_decl;
            }
            _funcs[FUNC] = fid;
            ++fid;
        }
    }
}

// -------------------------------------------------------------------------- //

CStmtPtr MainFunctionGenerator::init_contract(
    ContractDefinition const& _contract,
    shared_ptr<const CVarDecl> _id,
    shared_ptr<const CVarDecl> _state
)
{
    CFuncCallBuilder init_builder("Init_" + m_converter.get_name(_contract));

    if (auto ctor = _contract.constructor())
    {
        init_builder.push(make_shared<CReference>(_id->id()));
        init_builder.push(make_shared<CReference>(_state->id()));
        for (auto const param : ctor->parameters())
        {
            string const MSG
                = "Init field " + param->name() + " in " + _contract.name();
            init_builder.push(m_converter.get_nd_val(*param, MSG));
        }
    }

    return _id->assign(init_builder.merge_and_pop())->stmt();
}

// -------------------------------------------------------------------------- //

CBlockList MainFunctionGenerator::build_case(
    FunctionDefinition const& _def,
    map<VariableDeclaration const*, shared_ptr<CVarDecl>> & _args,
    shared_ptr<const CVarDecl> _id,
    shared_ptr<const CVarDecl> _state
)
{
    auto const& root = FunctionUtilities::extract_root(_def);

    CFuncCallBuilder call_builder(m_converter.get_name(root));
    call_builder.push(make_shared<CReference>(_id->id()));
    call_builder.push(make_shared<CReference>(_state->id()));

    CBlockList call_body;
    for (auto arg : _def.parameters())
    {
        string const MSG = "Set " + arg->name() + " for call " + _def.name();
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
    CBlockList & _stmts,
    shared_ptr<const CVarDecl> _state
)
{
    AddressType ADDR(StateMutability::Payable);
    IntegerType UINT256(256, IntegerType::Modifier::Unsigned);

    _stmts.push_back(_state->access("sender")->assign(
        TypeConverter::nd_val_by_simple_type(ADDR, "msg_sender"))->stmt()
    );
    _stmts.push_back(_state->access("value")->assign(
        TypeConverter::nd_val_by_simple_type(UINT256, "msg_value"))->stmt()
    );
    _stmts.push_back(_state->access("blocknum")->assign(
        TypeConverter::nd_val_by_simple_type(UINT256, "block_number"))->stmt()
    );

    _stmts.push_back(make_require(make_shared<CBinaryOp>(
        make_shared<CMemberAccess>(_state->access("sender"), "v"),
        "!=",
        Literals::ZERO
    )));
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
