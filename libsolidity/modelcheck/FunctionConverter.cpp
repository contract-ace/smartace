/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/FunctionConverter.h>

#include <libsolidity/modelcheck/BlockConverter.h>
#include <libsolidity/modelcheck/ExpressionConverter.h>
#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/Utility.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

const shared_ptr<CIdentifier> FunctionConverter::TMP =
    make_shared<CIdentifier>("tmp", false);

// -------------------------------------------------------------------------- //

FunctionConverter::FunctionConverter(
    ASTNode const& _ast,
	TypeConverter const& _converter,
    View _view,
    bool _fwd_dcl
): M_AST(_ast), M_CONVERTER(_converter), M_VIEW(_view), M_FWD_DCL(_fwd_dcl) {}

void FunctionConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    M_AST.accept(*this);
}

// -------------------------------------------------------------------------- //

bool FunctionConverter::visit(ContractDefinition const& _node)
{
    if (M_VIEW == View::INT) return true;

    string const CTRX_TYPE = M_CONVERTER.get_type(_node);
    string const CTRX_NAME = M_CONVERTER.get_name(_node);

    CParams params;
    if (auto ctor = _node.constructor())
    {
        params = generate_params(ctor->parameters(), &_node);
    }

    shared_ptr<CBlock> body;
    if (!M_FWD_DCL)
    {
        CBlockList stmts{make_shared<CVarDecl>(CTRX_TYPE, "tmp")};
        for (auto decl : _node.stateVariables())
        {
            auto member = make_shared<CMemberAccess>(TMP, "d_" + decl->name());
            CExprPtr init;
            if (decl->value())
            {
                init = ExpressionConverter(*decl->value(), {}, {}).convert();
            }
            else
            {
                init = M_CONVERTER.get_init_val(*decl);
            }
            auto asgn = make_shared<CAssign>(move(member), move(init));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        if (auto ctor = _node.constructor())
        {
            CArgList args;
            args.push_back(make_shared<CReference>(TMP));
            args.push_back(make_shared<CIdentifier>("state", true));
            for (auto decl : ctor->parameters())
            {
                args.push_back(make_shared<CIdentifier>(decl->name(), false));
            }
            auto ctor_call = make_shared<CFuncCall>(
                "Ctor_" + CTRX_NAME, move(args)
            );
            stmts.push_back(make_shared<CExprStmt>(ctor_call));
        }
        stmts.push_back(make_shared<CReturn>(TMP));

        body = make_shared<CBlock>(move(stmts));
    }

    auto id = make_shared<CVarDecl>(CTRX_TYPE, "Init_" + CTRX_NAME);
    CFuncDef init(id, move(params), move(body));
    (*m_ostream) << init;

    return true;
}

bool FunctionConverter::visit(StructDefinition const& _node)
{
    if (M_VIEW == View::EXT) return false;

    string const STRCT_NAME = M_CONVERTER.get_name(_node);
    string const STRCT_TYPE = M_CONVERTER.get_type(_node);

    vector<ASTPointer<VariableDeclaration>> initializable_members;
    for (auto const& member : _node.members())
    {
        if (has_simple_type(*member))
        {
            initializable_members.push_back(member);
        }
    }

    auto zero_id = make_shared<CVarDecl>(STRCT_TYPE, "Init_0_" + STRCT_NAME);
    auto init_id = make_shared<CVarDecl>(STRCT_TYPE, "Init_" + STRCT_NAME);
    auto nd_id = make_shared<CVarDecl>(STRCT_TYPE, "ND_" + STRCT_NAME);

    auto init_params = generate_params(initializable_members, nullptr);

    shared_ptr<CBlock> zero_body, init_body, nd_body;
    if (!M_FWD_DCL)
    {
        CBlockList stmts;
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp"));
        for (auto decl : _node.members())
        {
            auto member = make_shared<CMemberAccess>(TMP, "d_" + decl->name());
            auto init = M_CONVERTER.get_init_val(*decl);
            auto asgn = make_shared<CAssign>(move(member), move(init));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        zero_body = make_shared<CBlock>(move(stmts));

        auto zinit = make_shared<CFuncCall>("Init_0_" + STRCT_NAME, CArgList{});
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp", false, zinit));
        for (auto m : initializable_members)
        {
            auto member = make_shared<CMemberAccess>(TMP, "d_" + m->name());
            auto param = make_shared<CIdentifier>(m->name(), false);
            auto asgn = make_shared<CAssign>(move(member), move(param));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        init_body = make_shared<CBlock>(move(stmts));
        
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp"));
        for (auto decl : _node.members())
        {
            auto member = make_shared<CMemberAccess>(TMP, "d_" + decl->name());
            auto init = M_CONVERTER.get_nd_val(*decl);
            auto asgn = make_shared<CAssign>(move(member), move(init));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        nd_body = make_shared<CBlock>(move(stmts));
    }

    CFuncDef zero(zero_id, CParams{}, move(zero_body));
    CFuncDef init(init_id, move(init_params), move(init_body));
    CFuncDef nd(nd_id, CParams{}, move(nd_body));

    (*m_ostream) << zero << init << nd;

    return true;
}

bool FunctionConverter::visit(FunctionDefinition const& _node)
{
    const bool IS_PUB = _node.visibility() == Declaration::Visibility::Public;
    const bool IS_EXT = _node.visibility() == Declaration::Visibility::External;
    if (!(IS_PUB || IS_EXT) && M_VIEW == View::EXT) return false;
    if ((IS_PUB || IS_EXT) && M_VIEW == View::INT) return false;

    const bool IS_MUTABLE = _node.stateMutability() != StateMutability::Pure;
    auto params = generate_params(
        _node.parameters(), IS_MUTABLE ? _node.scope() : nullptr
    );

    shared_ptr<CBlock> body;
    if (!M_FWD_DCL)
    {
        body = BlockConverter(_node, M_CONVERTER).convert();
    }

    string const FUNC_TYPE = M_CONVERTER.get_type(_node);
    string const FUNC_NAME = M_CONVERTER.get_name(_node);
    auto id = make_shared<CVarDecl>(FUNC_TYPE, FUNC_NAME);
    CFuncDef func(id, move(params), move(body));

    (*m_ostream) << func;

    return false;
}

bool FunctionConverter::visit(ModifierDefinition const&) { return false; }

bool FunctionConverter::visit(Mapping const& _node)
{
    if (M_VIEW == View::EXT) return false;

    string const MAP_TYPE = M_CONVERTER.get_type(_node);
    string const KEY_TYPE = M_CONVERTER.get_type(_node.keyType());
    string const VAL_TYPE = M_CONVERTER.get_type(_node.valueType());
    string const MAP_NAME = M_CONVERTER.get_name(_node);

    auto arr = make_shared<CVarDecl>(MAP_TYPE, "a", true);
    auto indx = make_shared<CVarDecl>(KEY_TYPE, "idx");
    auto data = make_shared<CVarDecl>(VAL_TYPE, "d");

    auto zinit_id = make_shared<CVarDecl>(MAP_TYPE, "Init_0_" + MAP_NAME);
    auto nd_id = make_shared<CVarDecl>(MAP_TYPE, "ND_" + MAP_NAME);
    auto read_id = make_shared<CVarDecl>(VAL_TYPE, "Read_" + MAP_NAME);
    auto write_id = make_shared<CVarDecl>("void", "Write_" + MAP_NAME);
    auto ref_id = make_shared<CVarDecl>(VAL_TYPE, "Ref_" + MAP_NAME, true);

    shared_ptr<CBlock> zinit_body, nd_body, read_body, write_body, ref_body;
    if (!M_FWD_DCL)
    {
        auto tmp_set = make_shared<CMemberAccess>(TMP, "m_set");
        auto tmp_cur = make_shared<CMemberAccess>(TMP, "m_curr");
        auto tmp_dat = make_shared<CMemberAccess>(TMP, "d_");
        auto tmp_ndd = make_shared<CMemberAccess>(TMP, "d_nd");
        auto a_set = make_shared<CMemberAccess>(arr->id(), "m_set");
        auto a_cur = make_shared<CMemberAccess>(arr->id(), "m_curr");
        auto a_dat = make_shared<CMemberAccess>(arr->id(), "d_");
        auto a_ndd = make_shared<CMemberAccess>(arr->id(), "d_nd");

        auto init_set = TypeConverter::init_val_by_simple_type(BoolType{});
        auto init_key = M_CONVERTER.get_init_val(_node.keyType());
        auto init_val = M_CONVERTER.get_init_val(_node.valueType());
        auto nd_set = TypeConverter::nd_val_by_simple_type(BoolType{});
        auto nd_key = M_CONVERTER.get_nd_val(_node.keyType());
        auto nd_val = M_CONVERTER.get_nd_val(_node.valueType());

        auto true_val = make_shared<CIntLiteral>(1);
        auto update_curr = make_shared<CIf>(
            make_shared<CBinaryOp>(a_set, "==", init_set),
            make_shared<CBlock>(CBlockList{
                make_shared<CExprStmt>(make_shared<CAssign>(a_cur, indx->id())),
                make_shared<CExprStmt>(make_shared<CAssign>(a_set, true_val)),
            }
        ), nullptr);
        auto is_not_cur = make_shared<CBinaryOp>(indx->id(), "!=", a_cur);

        zinit_body = make_shared<CBlock>(CBlockList{
            make_shared<CVarDecl>(MAP_TYPE, "tmp", false, nullptr),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_set, init_set)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_cur, init_key)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_dat, init_val)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_ndd, init_val)),
            make_shared<CReturn>(TMP)
        });

        nd_body = make_shared<CBlock>(CBlockList{
            make_shared<CVarDecl>(MAP_TYPE, "tmp", false, nullptr),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_set, nd_set)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_cur, nd_key)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_dat, nd_val)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_ndd, init_val)),
            make_shared<CReturn>(TMP)
        });

        read_body = make_shared<CBlock>(CBlockList{
            update_curr,
            make_shared<CIf>(is_not_cur, make_shared<CReturn>(nd_val), nullptr),
            make_shared<CReturn>(a_dat)
        });

        write_body = make_shared<CBlock>(CBlockList{
            update_curr,
            make_shared<CIf>(
                make_shared<CBinaryOp>(indx->id(), "==", a_cur),
                make_shared<CExprStmt>(make_shared<CAssign>(a_dat, data->id())
            ), nullptr)
        });

        ref_body = make_shared<CBlock>(CBlockList{
            update_curr,
            make_shared<CIf>(is_not_cur, make_shared<CBlock>(CBlockList{
                make_shared<CExprStmt>(make_shared<CAssign>(a_ndd, nd_val)),
                make_shared<CReturn>(make_shared<CReference>(a_ndd))
            }), nullptr),
            make_shared<CReturn>(make_shared<CReference>(a_dat))
        });
    }

    CFuncDef zinit(zinit_id, CParams{}, move(zinit_body));
    CFuncDef nd(nd_id, CParams{}, move(nd_body));
    CFuncDef read(read_id, CParams{arr, indx}, move(read_body));
    CFuncDef write(write_id, CParams{arr, indx, data}, move(write_body));
    CFuncDef ref(ref_id, CParams{arr, indx}, move(ref_body));

    (*m_ostream) << zinit << nd << read << write << ref;

    return true;
}

// -------------------------------------------------------------------------- //

CParams FunctionConverter::generate_params(
    vector<ASTPointer<VariableDeclaration>> const& _args, ASTNode const* _scope
)
{
    CParams params;
    if (auto contract_scope = dynamic_cast<ContractDefinition const*>(_scope))
    {
        string const SELF_TYPE = M_CONVERTER.get_type(*contract_scope);
        auto const STATE_TYPE = "struct CallState";
        params.push_back(make_shared<CVarDecl>(SELF_TYPE, "self", true));
        params.push_back(make_shared<CVarDecl>(STATE_TYPE, "state", true));
    }
    for (auto arg : _args)
    {
        string const ARG_TYPE = M_CONVERTER.get_type(*arg);
        params.push_back(make_shared<CVarDecl>(ARG_TYPE, arg->name()));
    }
    return move(params);
}

// -------------------------------------------------------------------------- //

}
}
}
