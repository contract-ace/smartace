/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/translation/Function.h>

#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/translation/Block.h>
#include <libsolidity/modelcheck/translation/Mapping.h>
#include <libsolidity/modelcheck/translation/Expression.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
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
    size_t _map_k,
    View _view,
    bool _fwd_dcl
): M_AST(_ast),
   M_CONVERTER(_converter),
   M_MAP_K(_map_k),
   M_VIEW(_view),
   M_FWD_DCL(_fwd_dcl)
{
}

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
        FunctionConverter::ParamTmpl tmpl;
        tmpl.context = VarContext::STRUCT;
        tmpl.instrumentation = false;

        vector<FunctionConverter::ParamTmpl> param_tmpls;
        for (auto arg : ctor->parameters())
        {
            tmpl.decl = arg;
            param_tmpls.push_back(tmpl);
        }

        params = generate_params(param_tmpls, &_node);
    }

    shared_ptr<CBlock> body;
    if (!M_FWD_DCL)
    {
        CBlockList stmts{make_shared<CVarDecl>(CTRX_TYPE, "tmp")};
        {
            auto const NAME = ContractUtilities::balance_member();
            auto const* TYPE = ContractUtilities::balance_type();
            stmts.push_back(TMP->access(NAME)->assign(
                TypeConverter::init_val_by_simple_type(*TYPE)
            )->stmt());
        }
        for (auto decl : _node.stateVariables())
        {
            auto const DECLKIND = decl->annotation().type->category();
            if (DECLKIND == Type::Category::Contract) break;

            auto const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            CExprPtr init;
            if (decl->value())
            {
                init = ExpressionConverter(*decl->value(), {}, {}).convert();
                init = FunctionUtilities::try_to_wrap(*decl->type(), move(init));
            }
            else
            {
                init = M_CONVERTER.get_init_val(*decl);
            }

            auto member = TMP->access(NAME);
            stmts.push_back(member->assign(move(init))->stmt());
        }
        if (auto ctor = _node.constructor())
        {
            auto const& ROOT = FunctionUtilities::extract_root(*ctor);

            CFuncCallBuilder builder(M_CONVERTER.get_name(ROOT));
            builder.push(make_shared<CReference>(TMP));
            builder.push(make_shared<CIdentifier>("state", true));
            for (auto decl : ctor->parameters())
            {
                auto const NAME = VariableScopeResolver::rewrite(
                    decl->name(), false, VarContext::STRUCT
                );

                builder.push(make_shared<CIdentifier>(NAME, false));
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

    vector<FunctionConverter::ParamTmpl> initializable_members;
    {
        FunctionConverter::ParamTmpl tmpl;
        tmpl.context = VarContext::STRUCT;
        tmpl.instrumentation = false;

        for (auto const& member : _node.members())
        {
            if (has_simple_type(*member))
            {
                tmpl.decl = member;
                initializable_members.push_back(tmpl);
            }
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
            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            auto member = TMP->access(NAME);
            auto init = M_CONVERTER.get_init_val(*decl);
            stmts.push_back(member->assign(move(init))->stmt());
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        zero_body = make_shared<CBlock>(move(stmts));

        auto zinit = make_shared<CFuncCall>("Init_0_" + STRCT_NAME, CArgList{});
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp", false, zinit));
        for (auto m : initializable_members)
        {
            string const NAME = VariableScopeResolver::rewrite(
                m.decl->name(), false, VarContext::STRUCT
            );

            auto member = TMP->access(NAME);
            auto param = make_shared<CIdentifier>(NAME, false);
            stmts.push_back(member->assign(move(param))->stmt());
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        init_body = make_shared<CBlock>(move(stmts));
        
        stmts.push_back(make_shared<CVarDecl>(STRCT_TYPE, "tmp"));
        for (auto decl : _node.members())
        {
            string const MSG = "Set " + decl->name() + " in " + _node.name();
            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );
    
            auto member = TMP->access(NAME);
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

    vector<FunctionConverter::ParamTmpl> base_tmpl, mod_tmpl;
    {
        FunctionConverter::ParamTmpl tmpl;
        tmpl.context = VarContext::FUNCTION;

        for (auto arg : _node.parameters())
        {
            tmpl.decl = arg;

            tmpl.instrumentation = false;
            base_tmpl.push_back(tmpl);

            tmpl.instrumentation = true;
            mod_tmpl.push_back(tmpl);
        }
    }

    vector<CFuncDef> defs;

    {
        CParams params = generate_params(base_tmpl,_node.scope());

        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            body = FunctionBlockConverter(_node, M_CONVERTER).convert();
        }

        string const FUNC_TYPE = M_CONVERTER.get_type(_node);
        string const FUNC_NAME = M_CONVERTER.get_name(_node);
        auto id = make_shared<CVarDecl>(FUNC_TYPE, FUNC_NAME);
        defs.emplace_back(id, move(params), move(body));
    }

    CParams const mod_params = generate_params(mod_tmpl, _node.scope());
    for (size_t i = _node.modifiers().size(); i > 0; --i)
    {
        size_t const IDX = i - 1;
    
        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            body = ModifierBlockConverter(_node, IDX, M_CONVERTER).convert();
        }

        ModifierInvocation const& mod = *_node.modifiers()[IDX];

        string const MOD_TYPE = M_CONVERTER.get_type(mod);
        string const MOD_NAME = M_CONVERTER.get_name(mod);
        auto id = make_shared<CVarDecl>(MOD_TYPE, MOD_NAME);
        defs.emplace_back(id, mod_params, move(body));
    }

    for (auto const& def : defs)
    {
        (*m_ostream) << def;
    }

    return false;
}

bool FunctionConverter::visit(ModifierDefinition const&) { return false; }

bool FunctionConverter::visit(Mapping const& _node)
{
    if (M_VIEW == View::EXT) return false;

    MapGenerator gen(_node, M_MAP_K, M_CONVERTER);
    (*m_ostream) << gen.declare_zero_initializer(M_FWD_DCL)
                 << gen.declare_nd_initializer(M_FWD_DCL)
                 << gen.declare_read(M_FWD_DCL)
                 << gen.declare_write(M_FWD_DCL)
                 << gen.declare_ref(M_FWD_DCL);

    return true;
}

// -------------------------------------------------------------------------- //

CParams FunctionConverter::generate_params(
    vector<FunctionConverter::ParamTmpl> const& _args, ASTNode const* _scope
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
        string const ARG_TYPE = M_CONVERTER.get_type(*arg.decl);
        string const ARG_NAME = VariableScopeResolver::rewrite(
            arg.decl->name(), arg.instrumentation, arg.context
        );

        params.push_back(make_shared<CVarDecl>(ARG_TYPE, ARG_NAME));
    }
    return params;
}

// -------------------------------------------------------------------------- //

}
}
}
