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
    bool _forward_declare
): m_ast(_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void FunctionConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}

// -------------------------------------------------------------------------- //

bool FunctionConverter::visit(ContractDefinition const& _node)
{
    auto tranl = m_converter.translate(_node);

    CParams params;
    if (auto ctor = _node.constructor())
    {
        params = generate_params(ctor->parameters(), &_node);
    }

    shared_ptr<CBlock> body;
    if (!m_forward_declare)
    {
        // Declares a temporary contract to initialize.
        CBlockList stmts{make_shared<CVarDecl>(tranl.type, "tmp")};
        // Initializes all fields of the contract.
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
                auto decl_name = m_converter.translate(*decl).name;
                init = to_init_expr(decl_name, *decl->type());
            }
            auto asgn = make_shared<CAssign>(move(member), move(init));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        // Invokes the user-define constructor, if possible.
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
                "Ctor_" + tranl.name, move(args)
            );
            stmts.push_back(make_shared<CExprStmt>(ctor_call));
        }
        // Returns the temporary contract.
        stmts.push_back(make_shared<CReturn>(TMP));

        body = make_shared<CBlock>(move(stmts));
    }

    auto id = make_shared<CVarDecl>(tranl.type, "Init_" + tranl.name);
    CFuncDef init(id, move(params), move(body));
    (*m_ostream) << init;

    return true;
}

bool FunctionConverter::visit(StructDefinition const& _node)
{
    auto trasl = m_converter.translate(_node);
    vector<ASTPointer<VariableDeclaration>> initializable_members;
    for (auto const& member : _node.members())
    {
        if (is_basic_type(*member->type()))
        {
            initializable_members.push_back(member);
        }
    }

    auto zero_id = make_shared<CVarDecl>(trasl.type, "Init_0_" + trasl.name);
    auto init_id = make_shared<CVarDecl>(trasl.type, "Init_" + trasl.name);
    auto nd_id = make_shared<CVarDecl>(trasl.type, "ND_" + trasl.name);

    auto init_params = generate_params(initializable_members, nullptr);

    shared_ptr<CBlock> zero_body, init_body, nd_body;
    // Zero Initialization.
    if (!m_forward_declare)
    {
        CBlockList stmts;
        stmts.push_back(make_shared<CVarDecl>(trasl.type, "tmp"));
        for (auto decl : _node.members())
        {
            auto decl_name = m_converter.translate(*decl).name;
            auto member = make_shared<CMemberAccess>(TMP, "d_" + decl->name());
            auto init = to_init_expr(decl_name, *decl->type());
            auto asgn = make_shared<CAssign>(move(member), move(init));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        zero_body = make_shared<CBlock>(move(stmts));

        auto zinit = make_shared<CFuncCall>("Init_0_" + trasl.name, CArgList{});
        stmts.push_back(make_shared<CVarDecl>(trasl.type, "tmp", false, zinit));
        for (auto m : initializable_members)
        {
            auto member = make_shared<CMemberAccess>(TMP, "d_" + m->name());
            auto param = make_shared<CIdentifier>(m->name(), false);
            auto asgn = make_shared<CAssign>(move(member), move(param));
            stmts.push_back(make_shared<CExprStmt>(move(asgn)));
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        init_body = make_shared<CBlock>(move(stmts));
        
        stmts.push_back(make_shared<CVarDecl>(trasl.type, "tmp"));
        for (auto decl : _node.members())
        {
            auto decl_name = m_converter.translate(*decl).name;
            auto member = make_shared<CMemberAccess>(TMP, "d_" + decl->name());
            auto init = to_nd_expr(decl_name, *decl->type());
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
    bool is_mutable = _node.stateMutability() != StateMutability::Pure;
    auto params = generate_params(
        _node.parameters(), is_mutable ? _node.scope() : nullptr
    );

    shared_ptr<CBlock> body;
    if (!m_forward_declare)
    {
        body = BlockConverter(_node, m_converter).convert();
    }

    auto trasl = m_converter.translate(_node);
    auto id = make_shared<CVarDecl>(trasl.type, trasl.name);
    CFuncDef func(id, move(params), move(body));

    (*m_ostream) << func;

    return false;
}

bool FunctionConverter::visit(ModifierDefinition const&)
{
    return false;
}

bool FunctionConverter::visit(Mapping const& _node)
{
    Translation map_t = m_converter.translate(_node);
    Translation key_t = m_converter.translate(_node.keyType());
    Translation val_t = m_converter.translate(_node.valueType());

    auto const& k_type = *_node.keyType().annotation().type;
    auto const& v_type = *_node.valueType().annotation().type;

    auto arr = make_shared<CVarDecl>(map_t.type, "a", true);
    auto indx = make_shared<CVarDecl>(key_t.type, "idx");
    auto data = make_shared<CVarDecl>(val_t.type, "d");

    auto zinit_id = make_shared<CVarDecl>(map_t.type, "Init_0_" + map_t.name);
    auto nd_id = make_shared<CVarDecl>(map_t.type, "ND_" + map_t.name);
    auto read_id = make_shared<CVarDecl>(val_t.type, "Read_" + map_t.name);
    auto write_id = make_shared<CVarDecl>("void", "Write_" + map_t.name);
    auto ref_id = make_shared<CVarDecl>(val_t.type, "Ref_" + map_t.name, true);

    shared_ptr<CBlock> zinit_body, nd_body, read_body, write_body, ref_body;
    if (!m_forward_declare)
    {
        auto tmp_set = make_shared<CMemberAccess>(TMP, "m_set");
        auto tmp_cur = make_shared<CMemberAccess>(TMP, "m_curr");
        auto tmp_dat = make_shared<CMemberAccess>(TMP, "d_");
        auto tmp_ndd = make_shared<CMemberAccess>(TMP, "d_nd");
        auto a_set = make_shared<CMemberAccess>(arr->id(), "m_set");
        auto a_cur = make_shared<CMemberAccess>(arr->id(), "m_curr");
        auto a_dat = make_shared<CMemberAccess>(arr->id(), "d_");
        auto a_ndd = make_shared<CMemberAccess>(arr->id(), "d_nd");

        auto init_set = to_init_expr("int", BoolType{});
        auto init_key = to_init_expr(key_t.name, k_type);
        auto init_val = to_init_expr(val_t.name, v_type);
        auto nd_set = to_nd_expr("int", BoolType{});
        auto nd_key = to_nd_expr(key_t.name, k_type);
        auto nd_val = to_nd_expr(val_t.name, v_type);

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
            make_shared<CVarDecl>(map_t.type, "tmp", false, nullptr),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_set, init_set)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_cur, init_key)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_dat, init_val)),
            make_shared<CExprStmt>(make_shared<CAssign>(tmp_ndd, init_val)),
            make_shared<CReturn>(TMP)
        });

        nd_body = make_shared<CBlock>(CBlockList{
            make_shared<CVarDecl>(map_t.type, "tmp", false, nullptr),
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

bool FunctionConverter::is_basic_type(Type const& _type)
{
    switch (_type.category())
    {
        case Type::Category::Address:
        case Type::Category::Integer:
        case Type::Category::RationalNumber:
        case Type::Category::Bool:
        case Type::Category::FixedPoint:
        case Type::Category::Enum:
            return true;
        default:
            return false;
    }
}

CExprPtr FunctionConverter::to_init_expr(string const& _name, Type const& _type)
{
    return is_basic_type(_type)
        ? (CExprPtr)(make_shared<CIntLiteral>(0))
        : (CExprPtr)(make_shared<CFuncCall>("Init_0_" + _name, CArgList{}));
}

CExprPtr FunctionConverter::to_nd_expr(string const& _name, Type const& _type)
{
    return is_basic_type(_type)
        ? make_shared<CFuncCall>("ND_Init_Val", CArgList{})
        : make_shared<CFuncCall>("ND_" + _name, CArgList{});
}

// -------------------------------------------------------------------------- //

CParams FunctionConverter::generate_params(
    vector<ASTPointer<VariableDeclaration>> const& _args, ASTNode const* _scope
)
{
    CParams params;
    if (auto contract_scope = dynamic_cast<ContractDefinition const*>(_scope))
    {
        auto self_type = m_converter.translate(*contract_scope).type;
        auto state_type = "struct CallState";
        params.push_back(make_shared<CVarDecl>(self_type, "self", true));
        params.push_back(make_shared<CVarDecl>(state_type, "state", true));
    }
    for (auto arg : _args)
    {
        auto type = m_converter.translate(*arg).type;
        params.push_back(make_shared<CVarDecl>(type, arg->name()));
    }
    return move(params);
}

// -------------------------------------------------------------------------- //

}
}
}
