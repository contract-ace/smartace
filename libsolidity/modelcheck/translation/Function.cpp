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
#include <set>
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
    CallState const& _statedata,
	TypeConverter const& _converter,
    size_t _map_k,
    View _view,
    bool _fwd_dcl
): M_AST(_ast)
 , M_STATEDATA(_statedata)
 , M_CONVERTER(_converter)
 , M_MAP_K(_map_k)
 , M_VIEW(_view)
 , M_FWD_DCL(_fwd_dcl)
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
    if (!_node.isInterface())
    {
        if (!_node.isLibrary() && M_VIEW != View::INT)
        {
            handle_contract_initializer(_node, _node);
        }

        set<string> methods;
        for (auto contract : _node.annotation().linearizedBaseContracts)
        {
            for (auto func : contract->definedFunctions())
            {
                auto res = methods.insert(func->name());
                if (res.second)
                {
                    generate_function(FunctionSpecialization(*func, _node));
                }
            }
        }
    }

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

bool FunctionConverter::visit(FunctionDefinition const&) { return false; }

bool FunctionConverter::visit(ModifierDefinition const&) { return false; }

bool FunctionConverter::visit(Mapping const& _node)
{
    if (M_VIEW == View::EXT) return false;

    MapGenerator gen(_node, M_MAP_K, M_CONVERTER);
    (*m_ostream) << gen.declare_zero_initializer(M_FWD_DCL)
                 << gen.declare_nd_initializer(M_FWD_DCL)
                 << gen.declare_read(M_FWD_DCL)
                 << gen.declare_write(M_FWD_DCL);

    return false;
}

// -------------------------------------------------------------------------- //

CParams FunctionConverter::generate_params(
    vector<FunctionConverter::ParamTmpl> const& _args,
    ContractDefinition const* _scope
)
{
    CParams params;
    if (_scope && !_scope->isLibrary())
    {
        string const SELF_TYPE = M_CONVERTER.get_type(*_scope);
        params.push_back(make_shared<CVarDecl>(SELF_TYPE, "self", true));
        for (auto const& fld : M_STATEDATA.order())
        {
            params.push_back(make_shared<CVarDecl>(fld.tname, fld.name, false));
        }
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

bool FunctionConverter::record_pair(ASTNode const& inst, ASTNode const& user)
{
    if (!m_handled[make_pair(inst.id(), user.id())])
    {
        m_handled[make_pair(inst.id(), user.id())] = true;
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------- //

void FunctionConverter::generate_function(FunctionSpecialization const& _spec)
{
    auto const& FUNC = _spec.func();

    if (FUNC.isConstructor()) return;

    bool const IS_PUB = FUNC.visibility() == Declaration::Visibility::Public;
    bool const IS_EXT = FUNC.visibility() == Declaration::Visibility::External;
    if (!(IS_PUB || IS_EXT) && M_VIEW == View::EXT) return;
    if ((IS_PUB || IS_EXT) && M_VIEW == View::INT) return;

    handle_function(_spec);
}

// -------------------------------------------------------------------------- //

string FunctionConverter::handle_contract_initializer(
    ContractDefinition const& _initialized, ContractDefinition const& _for
)
{
    // Ensures this specialization is new.
    if (!record_pair(_initialized, _for)) return "";

    bool const IS_TOP_INIT_CALL = (_initialized.name() == _for.name());
    string const INIT_NAME = M_CONVERTER.get_name(_initialized);
    string const FOR_NAME = M_CONVERTER.get_name(_for);
    auto const* LOCAL_CTOR = _initialized.constructor();

    CParams params;
    string ctor_name;
    {
        vector<FunctionConverter::ParamTmpl> local_param_tmpl;
        if (LOCAL_CTOR)
        {
            FunctionConverter::ParamTmpl tmpl;
            tmpl.context = VarContext::STRUCT;
            tmpl.instrumentation = false;

            for (auto arg : LOCAL_CTOR->parameters())
            {
                tmpl.decl = arg;
                local_param_tmpl.push_back(tmpl);
            }

            ctor_name = handle_function(FunctionSpecialization(*LOCAL_CTOR, _for));
        }
        params = generate_params(local_param_tmpl, &_for);
    }

    auto self_ptr = params[0]->id();
    vector<CFuncCallBuilder> parent_initializers;
    for (auto spec : _initialized.baseContracts())
    {
        auto const* raw = spec->name().annotation().referencedDeclaration;
        auto const& parent = dynamic_cast<ContractDefinition const&>(*raw);

        if (parent.isInterface()) continue;

        auto parent_call = handle_contract_initializer(parent, _for);

        parent_initializers.emplace_back(parent_call);
        auto & builder = parent_initializers.back();

        builder.push(self_ptr);
        M_STATEDATA.compute_next_state_for(builder, false, nullptr);
        if (LOCAL_CTOR)
        {
            for (auto const CTOR_MOD : LOCAL_CTOR->modifiers())
            {
                auto const MOD_REF = CTOR_MOD->name()->annotation().referencedDeclaration;
                if (MOD_REF->name() == parent.name())
                {
                    if (auto const* CARGS = CTOR_MOD->arguments())
                    {
                        auto const& PARGS = parent.constructor()->parameters();
                        VariableScopeResolver resolver(CodeType::INITBLOCK);
                        for (size_t i = 0; i < PARGS.size(); ++i)
                        {
                            auto const& V = *CARGS->at(i);
                            auto const* T = PARGS[i]->annotation().type;
                            builder.push(
                                V, M_STATEDATA, M_CONVERTER, resolver, false, T
                            );
                        }
                    }
                    break;
                }
            }
        }
        if (spec->arguments())
        {
            for (size_t i = 0; i < spec->arguments()->size(); ++i)
            {
                auto const& arg = (*(*spec->arguments())[i]);
                auto const& param = (*parent.constructor()->parameters()[i]);
                auto const* type = param.annotation().type;
                builder.push(arg, M_STATEDATA, M_CONVERTER, {}, type);
            }
        }
    }

    shared_ptr<CBlock> body;
    if (!M_FWD_DCL)
    {
        CBlockList stmts;
        if (IS_TOP_INIT_CALL)
        {
            auto const NAME = ContractUtilities::balance_member();
            auto const* TYPE = ContractUtilities::balance_type();
            stmts.push_back(self_ptr->access(NAME)->assign(
                TypeConverter::init_val_by_simple_type(*TYPE)
            )->stmt());
        }
        for (auto initializer : parent_initializers)
        {
            stmts.push_back(initializer.merge_and_pop_stmt());
        }
        for (auto const* decl : _initialized.stateVariables())
        {
            auto const DECLKIND = decl->annotation().type->category();
            if (DECLKIND == Type::Category::Contract) continue;

            auto const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            CExprPtr v0;
            if (decl->value())
            {
                ExpressionConverter init_expr(*decl->value(), {}, {}, {});
                v0 = init_expr.convert();
                v0 = FunctionUtilities::try_to_wrap(*decl->type(), move(v0));
            }
            else
            {
                v0 = M_CONVERTER.get_init_val(*decl);
            }

            auto member = self_ptr->access(NAME);
            stmts.push_back(member->assign(move(v0))->stmt());
        }
        if (LOCAL_CTOR)
        {
            CFuncCallBuilder builder(ctor_name);
            builder.push(self_ptr);
            M_STATEDATA.compute_next_state_for(builder, false, nullptr);
            for (auto decl : LOCAL_CTOR->parameters())
            {
                auto const NAME = VariableScopeResolver::rewrite(
                    decl->name(), false, VarContext::STRUCT
                );

                builder.push(make_shared<CIdentifier>(NAME, false));
            }
            stmts.push_back(builder.merge_and_pop_stmt());
        }

        body = make_shared<CBlock>(move(stmts));
    }

    string fname = "Init_" + INIT_NAME;
    if (!IS_TOP_INIT_CALL)
    {
        fname += "_For_" + FOR_NAME;
    }

    auto id = make_shared<CVarDecl>("void", fname);
    CFuncDef init(id, move(params), move(body));
    (*m_ostream) << init;

    return fname;
}

// -------------------------------------------------------------------------- //

string FunctionConverter::handle_function(FunctionSpecialization const& _spec)
{
    // Ensures this specialization is new.
    if (!record_pair(_spec.func(), _spec.useby())) return "";

    // Bypasses pure virtual and uinterpreted functions.
    if (!_spec.func().isImplemented()) return "";

    // Determines return signature.
    auto const& FUNC = _spec.func();
    string ftype = "void";
    string fname = _spec.name();
    if (!FUNC.isConstructor())
    {
        ftype = M_CONVERTER.get_type(FUNC);
    }

    // Generates parameter list for all levels.
    vector<FunctionConverter::ParamTmpl> base_tmpl, mod_tmpl;
    {
        FunctionConverter::ParamTmpl tmpl;
        tmpl.context = VarContext::FUNCTION;

        for (auto arg : FUNC.parameters())
        {
            tmpl.decl = arg;

            tmpl.instrumentation = false;
            base_tmpl.push_back(tmpl);

            tmpl.instrumentation = true;
            mod_tmpl.push_back(tmpl);
        }
    }

    // Generates super function calls.
    if (auto superfunc = _spec.super())
    {
        handle_function(*superfunc);
    }

    // Filters modifiers from constructors.
    ModifierBlockConverter::Factory mods(FUNC, fname);

    // Generates a declaration for the base call.
    vector<CFuncDef> defs;
    {
        CParams params = generate_params(base_tmpl, &_spec.useby());

        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            FunctionBlockConverter cov(FUNC, M_STATEDATA, M_CONVERTER);
            cov.set_for(_spec);
            body = cov.convert();
        }

        string base_fname = fname;
        if (!mods.empty())
        {
            base_fname = FunctionUtilities::base_name(move(base_fname));
        }
        auto id = make_shared<CVarDecl>(ftype, move(base_fname));
        defs.emplace_back(id, move(params), move(body));
    }

    // Generates a declaration for each modifier.
    CParams const mod_params = generate_params(mod_tmpl, &_spec.useby());
    for (size_t i = mods.len(); i > 0; --i)
    {
        size_t const IDX = i - 1;
        string mod_name;
        if (IDX != 0)
        {
            mod_name = FunctionUtilities::modifier_name(fname, IDX);
        }
        else
        {
            mod_name = fname;
        }
        
        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            body = mods.generate(IDX, M_STATEDATA, M_CONVERTER).convert();
        }

        auto id = make_shared<CVarDecl>(ftype, move(mod_name));
        defs.emplace_back(id, mod_params, move(body));
    }

    // Prints each declaration.
    for (auto const& def : defs)
    {
        (*m_ostream) << def;
    }

    return fname;
}

// --------------------------------------------------------------------------

}
}
}
