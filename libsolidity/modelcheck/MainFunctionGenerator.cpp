/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
 */

#include <libsolidity/modelcheck/MainFunctionGenerator.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>

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
): NULL_LIT(make_shared<CIntLiteral>(0)), m_ast(_ast), m_converter(_converter)
{
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::print(std::ostream& _stream)
{
    CBlockList stmts;

    auto contracts = ASTNode::filteredNodes<ContractDefinition>(m_ast.nodes());
    if (contracts.size() > 1)
    {
        throw runtime_error("Multiple contracts not yet supported.");
    }

    auto const CURSTATE = make_shared<CVarDecl>("struct CallState", "curstate");
    auto const NXTSTATE = make_shared<CVarDecl>("struct CallState", "nxtstate");

    map<VariableDeclaration const*, shared_ptr<CVarDecl>> param_decls;
    map<ContractDefinition const*, shared_ptr<CVarDecl>> contract_decls;
    map<FunctionDefinition const*, uint64_t> func_id;
    analyze_decls(contracts, param_decls, contract_decls, func_id);

    for (auto param_pair : param_decls) stmts.push_back(param_pair.second);
    stmts.push_back(CURSTATE);
    stmts.push_back(CURSTATE->access("blocknum")->assign(
        make_shared<CIntLiteral>(0)
    )->stmt());
    stmts.push_back(NXTSTATE);
    for (auto contract_pair : contract_decls)
    {
        auto const& DECL = contract_pair.second;
        stmts.push_back(DECL);
        stmts.push_back(init_contract(*contract_pair.first, DECL, CURSTATE));
    }

    auto call_cases = make_shared<CSwitch>(
        get_nd(8, "Select next call"), CBlockList{make_require(NULL_LIT)}
    );
    for (auto const* CONTRACT : contracts)
    {
        auto CDECL = contract_decls[CONTRACT];
        for (auto const* FUNC : CONTRACT->definedFunctions())
        {
            if (FUNC->isConstructor()) continue;
            auto call_body = build_case(*FUNC, param_decls, CDECL, CURSTATE);
            call_cases->add_case(func_id[FUNC], move(call_body));
        }
    }

    stmts.push_back(make_shared<CWhileLoop>(
        make_shared<CBlock>(CBlockList{
            NXTSTATE->access("sender")->assign(
                get_nd(64, "Select the next sender's address")
            )->stmt(),
            NXTSTATE->access("value")->assign(
                get_nd(64, "Select the next message value")
            )->stmt(),
            NXTSTATE->access("blocknum")->assign(
                get_nd(64, "Select the next blocknum")
            )->stmt(),
            make_require(make_shared<CBinaryOp>(
                NXTSTATE->access("blocknum"), ">=", CURSTATE->access("blocknum")
            )),
            CURSTATE->assign(NXTSTATE->id())->stmt(),
            call_cases
        }),
        get_nd(8, "Select 0 to terminate"), false
    ));

    auto id = make_shared<CVarDecl>("void", "run_model");
    _stream << CFuncDef(id, CParams{}, make_shared<CBlock>(move(stmts)));
}

// -------------------------------------------------------------------------- //

CStmtPtr MainFunctionGenerator::make_require(CExprPtr _cond)
{
    auto fn = make_shared<CFuncCall>("sol_require", CArgList{_cond, NULL_LIT});
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
            if (FUNC->isConstructor()) continue;
            for (auto const PARAM : FUNC->parameters())
            {
                string const TYPE = m_converter.get_type(*PARAM);
                ostringstream param_name;
                param_name << "c" << i << "_f" << j << "_" << PARAM->name();

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
    CFuncCallBuilder call_builder(m_converter.get_name(_def));
    if (_def.stateMutability() != StateMutability::Pure)
    {
        call_builder.push(make_shared<CReference>(_id->id()));
        call_builder.push(make_shared<CReference>(_state->id()));
    }

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

CExprPtr MainFunctionGenerator::get_nd(unsigned int _bits, string const& _msg)
{
    return TypeConverter::nd_val_by_simple_type(IntegerType(_bits), _msg);
}

}
}
}
