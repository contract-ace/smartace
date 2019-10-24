/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/FunctionConverter.h>

#include <libsolidity/modelcheck/BlockConverter.h>
#include <libsolidity/modelcheck/ExpressionConverter.h>
#include <libsolidity/modelcheck/Mapping.h>
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
            auto member = TMP->access("d_" + decl->name());
            CExprPtr init;
            if (decl->value())
            {
                init = ExpressionConverter(*decl->value(), {}, {}).convert();
                if (is_wrapped_type(*decl->type()))
                {
                    init = make_shared<CFuncCall>(
                        "Init_" + M_CONVERTER.get_type(*decl), CArgList{init}
                    );
                }
            }
            else
            {
                init = M_CONVERTER.get_init_val(*decl);
            }
            stmts.push_back(member->assign(move(init))->stmt());
        }
        if (auto ctor = _node.constructor())
        {
            CFuncCallBuilder builder("Ctor_" + CTRX_NAME);
            builder.push(make_shared<CReference>(TMP));
            builder.push(make_shared<CIdentifier>("state", true));
            for (auto decl : ctor->parameters())
            {
                builder.push(make_shared<CIdentifier>(decl->name(), false));
            }
            stmts.push_back(builder.merge_and_pop_stmt());
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
        if (has_simple_type(*member)) initializable_members.push_back(member);
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
            auto member = TMP->access("d_" + decl->name());
            auto init = M_CONVERTER.get_init_val(*decl);
            stmts.push_back(member->assign(move(init))->stmt());
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        zero_body = make_shared<CBlock>(move(stmts));

        auto zinit = make_shared<CFuncCall>("Init_0_" + STRCT_NAME, CArgList{});
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp", false, zinit));
        for (auto m : initializable_members)
        {
            auto member = TMP->access("d_" + m->name());
            auto param = make_shared<CIdentifier>(m->name(), false);
            stmts.push_back(member->assign(move(param))->stmt());
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        init_body = make_shared<CBlock>(move(stmts));
        
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp"));
        for (auto decl : _node.members())
        {
            string const MSG = "Set " + decl->name() + " in " + _node.name();
            auto member = TMP->access("d_" + decl->name());
            auto init = M_CONVERTER.get_nd_val(*decl, MSG);
            stmts.push_back(member->assign(move(init))->stmt());
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

    // TODO(scottwe): There are some hard-coded .v's... This is fine as the
    //                indices are primitive. This isn't too maintainable if we
    //                change our primitive repr (this has happened already).
    shared_ptr<CBlock> zinit_body, nd_body, read_body, write_body, ref_body;
    if (!M_FWD_DCL)
    {
        string const BMSG = "Set if entry is selected in " + MAP_NAME;
        string const KMSG = "Set key in " + MAP_NAME;
        string const VMSG = "Set value in " + MAP_NAME;

        auto tmp_set = TMP->access(MappingUtilities::SET_FIELD);
        auto tmp_cur = TMP->access(MappingUtilities::CURR_FIELD);
        auto tmp_dat = TMP->access(MappingUtilities::DATA_FIELD);
        auto tmp_ndd = TMP->access(MappingUtilities::ND_FIELD);
        auto a_set = arr->access(MappingUtilities::SET_FIELD);
        auto a_cur = arr->access(MappingUtilities::CURR_FIELD);
        auto a_dat = arr->access(MappingUtilities::DATA_FIELD);
        auto a_ndd = arr->access(MappingUtilities::ND_FIELD);

        auto init_set = TypeConverter::init_val_by_simple_type(BoolType{});
        auto init_key = M_CONVERTER.get_init_val(_node.keyType());
        auto init_val = M_CONVERTER.get_init_val(_node.valueType());
        auto nd_set = TypeConverter::nd_val_by_simple_type(BoolType{}, BMSG);
        auto nd_key = M_CONVERTER.get_nd_val(_node.keyType(), KMSG);
        auto nd_val = M_CONVERTER.get_nd_val(_node.valueType(), VMSG);

        auto true_val = make_shared<CIntLiteral>(1);
        auto true_adt = make_shared<CFuncCall>(
            "Init_" + TypeConverter::get_simple_ctype(BoolType{}),
            CArgList{true_val}
        );
        auto update_curr = make_shared<CIf>(
            make_shared<CBinaryOp>(a_set->access("v"), "!=", true_val),
            make_shared<CBlock>(CBlockList{
                a_cur->assign(indx->id())->stmt(),
                a_set->assign(true_adt)->stmt()
            }
        ), nullptr);
        auto is_not_cur = make_shared<CBinaryOp>(
            indx->id()->access("v"), "!=", a_cur->access("v")
        );

        zinit_body = make_shared<CBlock>(CBlockList{
            make_shared<CVarDecl>(MAP_TYPE, "tmp", false, nullptr),
            tmp_set->assign(init_set)->stmt(),
            tmp_cur->assign(init_key)->stmt(),
            tmp_dat->assign(init_val)->stmt(),
            tmp_ndd->assign(init_val)->stmt(),
            make_shared<CReturn>(TMP)
        });

        nd_body = make_shared<CBlock>(CBlockList{
            make_shared<CVarDecl>(MAP_TYPE, "tmp", false, nullptr),
            tmp_set->assign(nd_set)->stmt(),
            tmp_cur->assign(nd_key)->stmt(),
            tmp_dat->assign(nd_val)->stmt(),
            tmp_ndd->assign(init_val)->stmt(),
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
                make_shared<CBinaryOp>(
                    indx->id()->access("v"), "==", a_cur->access("v")
                ),
                a_dat->assign(data->id())->stmt(),
                nullptr
            )
        });

        ref_body = make_shared<CBlock>(CBlockList{
            update_curr,
            make_shared<CIf>(is_not_cur, make_shared<CBlock>(CBlockList{
                a_ndd->assign(nd_val)->stmt(),
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
    return params;
}

// -------------------------------------------------------------------------- //

}
}
}
