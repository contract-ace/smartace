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
    map<FunctionDefinition const*, uint64_t> func_id;
    analyze_decls(contracts, param_decls, func_id);

    for (auto param_pair : param_decls) stmts.push_back(param_pair.second);
    stmts.push_back(CURSTATE);
    stmts.push_back(NXTSTATE);

    auto call_cases = make_shared<CSwitch>(
        TypeConverter::nd_val_by_simple_type(IntegerType(8)),
        CBlockList{make_require(NULL_LIT)}
    );
    for (auto const* CONTRACT : contracts)
    {
        string const TYPE = m_converter.get_type(*CONTRACT);
        string const NAME = m_converter.get_name(*CONTRACT);
        auto decl = make_shared<CVarDecl>(TYPE, "contract");
        stmts.push_back(decl);
        stmts.push_back(make_shared<CExprStmt>(make_shared<CAssign>(
            decl->id(), make_shared<CFuncCall>("Init_" + NAME, CArgList{
                make_shared<CReference>(decl->id()),
                make_shared<CReference>(CURSTATE->id())
            })
        )));

        for (auto const* FUNC : CONTRACT->definedFunctions())
        {
            if (FUNC->isConstructor()) continue;
            CFuncCallBuilder call_builder(m_converter.get_name(*FUNC));
            if (FUNC->stateMutability() != StateMutability::Pure)
            {
                call_builder.push(make_shared<CReference>(decl->id()));
                call_builder.push(make_shared<CReference>(CURSTATE->id()));
            }

            CBlockList call_body;
            for (auto arg : FUNC->parameters())
            {
                call_body.push_back(make_shared<CExprStmt>(
                    make_shared<CAssign>(
                        param_decls[arg.get()]->id(),
                        m_converter.get_nd_val(*arg)
                    )
                ));
                call_builder.push(param_decls[arg.get()]->id());
            }
            call_body.push_back(make_shared<CExprStmt>(
                call_builder.merge_and_pop())
            );
            call_body.push_back(make_shared<CBreak>());
            call_cases->add_case(func_id[FUNC], call_body);
        }
    }

    stmts.push_back(make_shared<CWhileLoop>(
        make_shared<CBlock>(CBlockList{
            call_cases,
            make_shared<CExprStmt>(make_shared<CAssign>(
                make_shared<CMemberAccess>(NXTSTATE->id(), "sender"),
                TypeConverter::nd_val_by_simple_type(IntegerType(64))
            )),
            make_shared<CExprStmt>(make_shared<CAssign>(
                make_shared<CMemberAccess>(NXTSTATE->id(), "value"),
                TypeConverter::nd_val_by_simple_type(IntegerType(64))
            )),
            make_shared<CExprStmt>(make_shared<CAssign>(
                make_shared<CMemberAccess>(NXTSTATE->id(), "blocknum"),
                TypeConverter::nd_val_by_simple_type(IntegerType(64))
            )),
            make_require(make_shared<CBinaryOp>(
                make_shared<CMemberAccess>(NXTSTATE->id(), "blocknum"),
                ">=",
                make_shared<CMemberAccess>(CURSTATE->id(), "blocknum")
            )),
            make_shared<CExprStmt>(make_shared<CAssign>(
                CURSTATE->id(), NXTSTATE->id()
            ))
        }),
        TypeConverter::nd_val_by_simple_type(IntegerType(8)), false
    ));

    auto id = make_shared<CVarDecl>("void", "main");
    _stream << CFuncDef(id, CParams{}, make_shared<CBlock>(move(stmts)));
}

// -------------------------------------------------------------------------- //

CStmtPtr MainFunctionGenerator::make_require(CExprPtr _cond)
{
    auto fn = make_shared<CFuncCall>("sol_require", CArgList{_cond, NULL_LIT});
    return make_shared<CExprStmt>(move(fn));
}

// -------------------------------------------------------------------------- //

void MainFunctionGenerator::analyze_decls(
    vector<ContractDefinition const*> const& _contracts, 
    map<VariableDeclaration const*, shared_ptr<CVarDecl>> & _dcls,
    map<FunctionDefinition const*, uint64_t> & _funcs)
{
    uint32_t fid = 0;
    for (unsigned int i = 0; i < _contracts.size(); ++i)
    {
        auto const& CONTRACT = *_contracts[i];
        for (unsigned int j = 0; j < CONTRACT.definedFunctions().size(); ++j)
        {
            auto const* FUNC = CONTRACT.definedFunctions()[j];
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

}
}
}
