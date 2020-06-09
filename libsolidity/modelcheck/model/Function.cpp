#include <libsolidity/modelcheck/model/Function.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractDependance.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/model/Block.h>
#include <libsolidity/modelcheck/model/Mapping.h>
#include <libsolidity/modelcheck/model/Expression.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/Types.h>

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
    ContractDependance const& _dependance,
    CallState const& _statedata,
    NewCallGraph const& _newcalls,
	TypeAnalyzer const& _converter,
    bool _add_sums,
    size_t _map_k,
    View _view,
    bool _fwd_dcl
): M_AST(_ast)
 , M_DEPENDANCE(_dependance)
 , M_STATEDATA(_statedata)
 , M_NEWCALLS(_newcalls)
 , M_CONVERTER(_converter)
 , M_ADD_SUMS(_add_sums)
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
    // Libraries are handled differently from member functions.
    if (_node.isLibrary())
    {
        // Generates the library methods.
        for (auto FUNC : _node.definedFunctions())
        {
            generate_function(FunctionSpecialization(*FUNC, _node));
        }
    }
    else if (M_DEPENDANCE.is_deployed(&_node))
    {
        // Generates initializer of the contract when in correct view.
        if (M_VIEW != View::INT)
        {
            handle_contract_initializer(_node, _node);
        }

        // Performs special handling of the fallback method.
        if (auto fallback = _node.fallbackFunction())
        {
            handle_function(FunctionSpecialization(*fallback), "void", false);
        }

        // Generates the dependance-specified interface for the contract.
        for (auto func : M_DEPENDANCE.get_interface(&_node))
        {
            for (auto func : M_DEPENDANCE.get_superchain(&_node, func))
            {
                generate_function(FunctionSpecialization(*func, _node));
            }
        }
    }

    return true;
}

bool FunctionConverter::visit(StructDefinition const& _node)
{
    if (M_VIEW == View::EXT) return false;

    InitFunction initdata(M_CONVERTER, _node);

    SolDeclList basic_decls;
    for (auto const& member : _node.members())
    {
        if (has_simple_type(*member)) basic_decls.push_back(member);
    }

    auto init_params = generate_params(basic_decls, nullptr, nullptr);

    shared_ptr<CBlock> zero_body, init_body;
    if (!M_FWD_DCL)
    {
        string const STRUCT_T = M_CONVERTER.get_type(_node);

        CBlockList stmts;
        stmts.push_back(make_shared<CVarDecl>(STRUCT_T, "tmp"));
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

        auto zinit = initdata.defaulted();
        stmts.push_back(make_shared<CVarDecl>(STRUCT_T, "tmp", false, zinit));
        for (auto decl : basic_decls)
        {
            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            auto member = TMP->access(NAME);
            auto param = make_shared<CIdentifier>(NAME, false);
            stmts.push_back(member->assign(move(param))->stmt());
        }
        stmts.push_back(make_shared<CReturn>(TMP));
        init_body = make_shared<CBlock>(move(stmts));
    }

    CFuncDef zero(initdata.default_id(), CParams{}, move(zero_body));
    CFuncDef init(initdata.call_id(), move(init_params), move(init_body));

    (*m_ostream) << zero << init;

    return true;
}

bool FunctionConverter::visit(FunctionDefinition const&) { return false; }

bool FunctionConverter::visit(ModifierDefinition const&) { return false; }

bool FunctionConverter::visit(Mapping const& _node)
{
    if (M_VIEW == View::EXT) return false;

    MapGenerator gen(_node, M_ADD_SUMS, M_MAP_K, M_CONVERTER);
    (*m_ostream) << gen.declare_zero_initializer(M_FWD_DCL)
                 << gen.declare_read(M_FWD_DCL)
                 << gen.declare_write(M_FWD_DCL)
                 << gen.declare_set(M_FWD_DCL);

    return false;
}

// -------------------------------------------------------------------------- //

CParams FunctionConverter::generate_params(
    SolDeclList const& _decls,
    ContractDefinition const* _scope,
    ASTPointer<VariableDeclaration> _dest,
    VarContext _context,
    bool _instrumeneted
)
{
    CParams params;
    if (_scope && !_scope->isLibrary())
    {
        string const SELF_TYPE = M_CONVERTER.get_type(*_scope);
        params.push_back(make_shared<CVarDecl>(SELF_TYPE, "self", true));
        for (auto const& fld : M_STATEDATA.order())
        {
            params.push_back(make_shared<CVarDecl>(
                fld.type_name, fld.name, false
            ));
        }
    }
    for (size_t i = 0; i < _decls.size(); ++i)
    {
        auto const& DECL = *_decls[i];
    
        string type = M_CONVERTER.get_type(DECL);
    
        string name = DECL.name();
        if (name.empty()) name = "var" + to_string(i);
        name = VariableScopeResolver::rewrite(name, _instrumeneted, _context);

		bool is_ref = DECL.referenceLocation() == VariableDeclaration::Storage;
        params.push_back(make_shared<CVarDecl>(move(type), move(name), is_ref));
    }
    if (_dest)
    {
        params.push_back(make_shared<CVarDecl>(
            M_CONVERTER.get_type(M_NEWCALLS.specialize(*_dest)),
            InitFunction::INIT_VAR,
            true
        ));
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
    if (FUNC.isFallback()) return;

    bool const IS_PUB = FUNC.visibility() == Declaration::Visibility::Public;
    bool const IS_EXT = FUNC.visibility() == Declaration::Visibility::External;
    if (!(IS_PUB || IS_EXT) && M_VIEW == View::EXT) return;
    if ((IS_PUB || IS_EXT) && M_VIEW == View::INT) return;

    auto rvs = FUNC.returnParameters();
    if (!rvs.empty() && rvs[0]->type()->category() == Type::Category::Contract)
    {
        if (M_NEWCALLS.retval_is_allocated(*rvs[0]))
        {
            handle_function(_spec, "void", false);
        }
        else
        {
            handle_function(_spec, M_CONVERTER.get_type(FUNC), true);
        }
    }
    else
    {
        handle_function(_spec, M_CONVERTER.get_type(FUNC), false);
    }
}

// -------------------------------------------------------------------------- //

string FunctionConverter::handle_contract_initializer(
    ContractDefinition const& _initialized, ContractDefinition const& _for
)
{
    auto const NAME = InitFunction(M_CONVERTER, _initialized, _for).call_name();
    auto const* LOCAL_CTOR = _initialized.constructor();

    // Ensures this specialization is new.
    if (!record_pair(_initialized, _for)) return NAME;

    CParams params;
    string ctor_name;
    {
        SolDeclList decls;
        if (LOCAL_CTOR)
        {
            decls = LOCAL_CTOR->parameters();

            ctor_name = handle_function(
                FunctionSpecialization(*LOCAL_CTOR, _for), "void", false
            );
        }
        params = generate_params(decls, &_for, nullptr);
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
                                V, M_CONVERTER, M_STATEDATA, resolver, false, T
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
                builder.push(arg, M_CONVERTER, M_STATEDATA, {}, type);
            }
        }
    }

    shared_ptr<CBlock> body;
    if (!M_FWD_DCL)
    {
        CBlockList stmts;
        if (&_initialized == &_for)
        {
            auto const NAME = ContractUtilities::balance_member();
            auto const* TYPE = ContractUtilities::balance_type();
            stmts.push_back(self_ptr->access(NAME)->assign(
                TypeAnalyzer::init_val_by_simple_type(*TYPE)
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
                ExpressionConverter init_expr(
                    *decl->value(), {}, M_STATEDATA, {}
                );
                v0 = init_expr.convert();
                v0 = InitFunction::wrap(*decl->type(), move(v0));
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

    auto id = make_shared<CVarDecl>("void", NAME);
    CFuncDef init(id, move(params), move(body));
    (*m_ostream) << init;

    return NAME;
}

// -------------------------------------------------------------------------- //

string FunctionConverter::handle_function(
    FunctionSpecialization const& _spec, string _rv_type, bool _rv_is_ptr
)
{
    // Ensures this specialization is new.
    string fname = _spec.name(0);
    if (!record_pair(_spec.func(), _spec.use_by())) return fname;

    // Determines if a contract initialization destination is required.
    ASTPointer<VariableDeclaration> dest;
    auto rvs = _spec.func().returnParameters();
    if (!rvs.empty() && M_NEWCALLS.retval_is_allocated(*rvs[0]))
    {
        dest = rvs[0];
    }

    // Filters modifiers from constructors.
    ModifierBlockConverter::Factory mods(_spec);

    // Generates a declaration for the base call.
    auto const CONTEXT = VarContext::FUNCTION;
    vector<CFuncDef> defs;
    {
        CParams params = generate_params(
            _spec.func().parameters(), &_spec.use_by(), dest, CONTEXT
        );

        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            FunctionBlockConverter cov(
                _spec.func(), M_CONVERTER, M_STATEDATA, M_NEWCALLS
            );

            cov.set_for(_spec);
            body = cov.convert();
        }

        string base_fname = _spec.name(mods.len());
        auto id = make_shared<CVarDecl>(_rv_type, move(base_fname), _rv_is_ptr);
        defs.emplace_back(id, move(params), move(body));
    }

    // Generates a declaration for each modifier.
    CParams const mod_params = generate_params(
        _spec.func().parameters(), &_spec.use_by(), dest, CONTEXT, true
    );
    for (size_t i = mods.len(); i > 0; --i)
    {
        size_t const IDX = i - 1;

        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            body = mods.generate(
                IDX, M_CONVERTER, M_STATEDATA, M_NEWCALLS
            ).convert();
        }

        auto id = make_shared<CVarDecl>(_rv_type, _spec.name(IDX), _rv_is_ptr);
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
