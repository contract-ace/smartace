#include <libsolidity/modelcheck/model/Function.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Library.h>
#include <libsolidity/modelcheck/analysis/Structure.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/model/Block.h>
#include <libsolidity/modelcheck/model/Mapping.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/model/Expression.h>
#include <libsolidity/modelcheck/utils/AST.h>
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
    shared_ptr<AnalysisStack> _stack,
    shared_ptr<NondetSourceRegistry> _nd_reg,
    bool _add_sums,
    size_t _map_k,
    View _view,
    bool _fwd_dcl
): M_ADD_SUMS(_add_sums)
 , M_MAP_K(_map_k)
 , M_VIEW(_view)
 , M_FWD_DCL(_fwd_dcl)
 , m_stack(_stack)
 , m_nd_reg(_nd_reg)
{
}

void FunctionConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);

    // Prints all library methods.
    for (auto library : m_stack->libraries()->view())
    {
        // Prints structure specific methods.
        for (auto structure : library->structures())
        {
            generate_structure(*structure);
        }

        // Prints user-defined methods.
        for (auto func : library->functions())
        {
            generate_function(FunctionSpecialization(*func, *library->raw()));
        }
    }

    // Prints all contract methods
    for (auto contract : m_stack->model()->view())
    {
        // Generates utility functions for structures.
        for (auto structure : contract->structures())
        {
            generate_structure(*structure);
        }

        for (auto mapping : contract->mappings())
        {
            generate_mapping(*mapping);
        }

        // Prints initializer.
        handle_contract_initializer(*contract, contract->tree());

        // Performs special handling of the fallback method.
        if (auto fallback = contract->fallback())
        {
            FunctionSpecialization const SPECS(*fallback, *contract->raw());
            handle_function(SPECS, "void", false);
        }

        // Prints all user-defined public methods.
        for (auto func : contract->interface())
        {
            generate_method(*contract, *func);
        }

        // Prints all user-defined internal methods.
        for (auto func : m_stack->calls()->internals(*contract))
        {
            generate_method(*contract, *func);
        }
    }
}

// -------------------------------------------------------------------------- //

CParams FunctionConverter::generate_params(
    SolDeclList const& _rvs,
    SolDeclList const& _decls,
    ContractDefinition const* _scope,
    ASTPointer<VariableDeclaration> _dest,
    VarContext _context,
    bool _instrumeneted
)
{
    CParams params;
    bool is_scoped = (_scope != nullptr);
    bool is_contract = (is_scoped && !_scope->isLibrary());

    // Pushes context for contract methods.
    if (is_contract)
    {
        string const SELF_TYPE = m_stack->types()->get_type(*_scope);
        params.push_back(make_shared<CVarDecl>(SELF_TYPE, "self", true));
    }

    // Pushes environment variables, while respecting scope rules.
    if (is_scoped)
    {
        for (auto const& fld: m_stack->environment()->order())
        {
            if (!is_contract && fld.contract_only) continue;
            params.push_back(make_shared<CVarDecl>(
                fld.type_name, fld.name, false
            ));
        }
    }

    // Pushes return values.
    for (size_t i = 1; i < _rvs.size(); ++i)
    {
        auto const& RV = *_rvs[i];

        string type = m_stack->types()->get_type(RV);

        string name = RV.name();
        if (name.empty())
        {
            // There are no collisions since variable names start with letters.
            name = to_string(i);
        }
        name = VariableScopeResolver::rewrite(name, _instrumeneted, _context);

        params.push_back(make_shared<CVarDecl>(move(type), move(name), true));
    }

    // Pushes arguments.
    for (size_t i = 0; i < _decls.size(); ++i)
    {
        auto const& DECL = *_decls[i];
    
        string type = m_stack->types()->get_type(DECL);
    
        string name = DECL.name();
        if (name.empty()) name = "var" + to_string(i);
        name = VariableScopeResolver::rewrite(name, _instrumeneted, _context);

        bool const IS_REF = decl_is_ref(DECL);
        params.push_back(make_shared<CVarDecl>(move(type), move(name), IS_REF));
    }

    // Pushes destination (for initializers, etc.).
    if (_dest)
    {
        auto const& spec = m_stack->allocations()->specialize(*_dest);
        params.push_back(make_shared<CVarDecl>(
            m_stack->types()->get_type(spec), InitFunction::INIT_VAR, true
        ));
    }

    return params;
}

// -------------------------------------------------------------------------- //

void FunctionConverter::expand_default_init(
    VariableDeclaration const* _decl,
    CBlockList & _stmts,
    shared_ptr<CIdentifier> _self
) const
{
    auto const DECLKIND = _decl->annotation().type->category();
    if (DECLKIND == Type::Category::Contract) return;

    if (_decl->isConstant()) return;

    auto const NAME = VariableScopeResolver::rewrite(
            _decl->name(), false, VarContext::STRUCT
    );

    CExprPtr v0;
    if (_decl->value())
    {
        v0 = ExpressionConverter(*_decl->value(), m_stack, {}).convert();
        v0 = InitFunction::wrap(*_decl->type(), move(v0));
    }
    else
    {
        v0 = m_stack->types()->get_init_val(*_decl);
    }

    auto member = _self->access(NAME);
    _stmts.push_back(member->assign(move(v0))->stmt());
}

// -------------------------------------------------------------------------- //

void FunctionConverter::generate_mapping(Mapping const& _map)
{
    if (M_VIEW == View::EXT) return;
    if (!m_visited.insert(make_pair(&_map, nullptr)).second) return;

    MapGenerator gen(_map, M_ADD_SUMS, M_MAP_K, *m_stack->types());
    (*m_ostream) << gen.declare_zero_initializer(M_FWD_DCL)
                 << gen.declare_read(M_FWD_DCL)
                 << gen.declare_write(M_FWD_DCL);
}

// -------------------------------------------------------------------------- //

void FunctionConverter::generate_structure(Structure const& _struct)
{
    if (M_VIEW == View::EXT) return;
    if (!m_visited.insert(make_pair(_struct.raw(), nullptr)).second) return;

    for (auto mapping : _struct.mappings())
    {
        generate_mapping(*mapping);
    }

    InitFunction const INIT_DATA(*m_stack->types(), *_struct.raw());

    SolDeclList basic_decls;
    for (auto const& field : _struct.fields())
    {
        if (has_simple_type(*field)) basic_decls.push_back(field);
    }

    auto init_params = generate_params({}, basic_decls, nullptr, nullptr);

    shared_ptr<CBlock> zero_body, init_body, nondet_body;
    if (!M_FWD_DCL)
    {
        CBlockList zero_stmts, init_stmts, nondet_stmts;
        string const STRUCT_T = m_stack->types()->get_type(*_struct.raw());

        // Default case.
        zero_stmts.push_back(make_shared<CVarDecl>(STRUCT_T, "tmp"));
        for (auto field : _struct.fields())
        {
            expand_default_init(field.get(), zero_stmts, TMP);
        }
        zero_stmts.push_back(make_shared<CReturn>(TMP));
        zero_body = make_shared<CBlock>(move(zero_stmts));

        // Init case.
        auto zinit = INIT_DATA.defaulted();
        init_stmts.push_back(make_shared<CVarDecl>(STRUCT_T, "tmp", false, zinit));
        for (auto field : basic_decls)
        {
            string const NAME = VariableScopeResolver::rewrite(
                field->name(), false, VarContext::STRUCT
            );

            auto member = TMP->access(NAME);
            auto param = make_shared<CIdentifier>(NAME, false);
            init_stmts.push_back(member->assign(move(param))->stmt());
        }
        init_stmts.push_back(make_shared<CReturn>(TMP));
        init_body = make_shared<CBlock>(move(init_stmts));

        // Nondet case.
        nondet_stmts.push_back(make_shared<CVarDecl>(STRUCT_T, "tmp"));
        for (auto field : _struct.fields())
        {
            string const NAME = VariableScopeResolver::rewrite(
                field->name(), false, VarContext::STRUCT
            );
            string const MSG = INIT_DATA.name() + ":" + field->name();

            auto member = TMP->access(NAME);
            if (auto init = m_nd_reg->val(*field->typeName(), MSG))
            {
                nondet_stmts.push_back(member->assign(move(init))->stmt());
            }
        }
        nondet_stmts.push_back(make_shared<CReturn>(TMP));
        nondet_body = make_shared<CBlock>(move(nondet_stmts));
    }

    CFuncDef zero(INIT_DATA.default_id(), CParams{}, move(zero_body));
    CFuncDef init(INIT_DATA.call_id(), move(init_params), move(init_body));
    CFuncDef nondet(INIT_DATA.nd_id(), CParams{}, move(nondet_body));

    (*m_ostream) << zero << init << nondet;
}

// -------------------------------------------------------------------------- //

void FunctionConverter::generate_method(
    FlatContract const& _contract, FunctionDefinition const& _func
)
{
    for (auto func : m_stack->calls()->super_calls(_contract, _func))
    {
        generate_function(FunctionSpecialization(*func, *_contract.raw()));
    }
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
        if (m_stack->allocations()->retval_is_allocated(*rvs[0]))
        {
            handle_function(_spec, "void", false);
        }
        else
        {
            handle_function(_spec, m_stack->types()->get_type(FUNC), true);
        }
    }
    else
    {
        handle_function(_spec, m_stack->types()->get_type(FUNC), false);
    }
}

// -------------------------------------------------------------------------- //

string FunctionConverter::handle_contract_initializer(
    FlatContract const& _for, InheritanceTree const& _tree
)
{
    InitFunction const INIT_DATA(*m_stack->types(), *_tree.raw(), *_for.raw());
    auto const NAME = INIT_DATA.call_name();

    if (M_VIEW == View::INT) return NAME;
    if (!m_visited.insert(make_pair(_tree.raw(), &_for)).second) return NAME;

    CParams params;
    string ctor_name;
    {
        SolDeclList decls;
        if (auto ctor = _tree.constructor())
        {
            ctor_name = handle_function(
                FunctionSpecialization(*ctor, *_for.raw()), "void", false
            );
            decls = ctor->parameters();
        }
        params = generate_params({}, decls, _for.raw(), nullptr);
    }

    auto self_ptr = params[0]->id();
    vector<CFuncCallBuilder> parent_initializers;
    for (auto call_data : _tree.baseContracts())
    {
        auto call = handle_contract_initializer(_for, *call_data.parent);

        parent_initializers.emplace_back(call);
        auto & builder = parent_initializers.back();

        builder.push(self_ptr);
        m_stack->environment()->compute_next_state_for(
            builder, false, true, nullptr
        );

        if (auto ctor = call_data.parent->constructor())
        {
            VariableScopeResolver resolver(CodeType::INITBLOCK);
            for (size_t i = 0; i < call_data.args.size(); ++i)
            {
                auto const& V = *call_data.args.at(i);
                auto const* T = ctor->parameters()[i]->annotation().type;
                builder.push(V, m_stack, resolver, false, T);
            }
        }
    }

    shared_ptr<CBlock> body;
    if (!M_FWD_DCL)
    {
        CBlockList stmts;
        if (_tree.raw() == _for.raw())
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
        for (auto const* decl : _tree.decls())
        {
            expand_default_init(decl, stmts, self_ptr);
        }
        if (auto ctor = _tree.constructor())
        {
            CFuncCallBuilder builder(ctor_name);
            builder.push(self_ptr);
            m_stack->environment()->compute_next_state_for(
                builder, false, true, nullptr
            );
            for (auto decl : ctor->parameters())
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
    auto const& FUNC = _spec.func();
    auto const& USER = _spec.use_by();
    if (!m_visited.insert(make_pair(&FUNC, &USER)).second) return _spec.name(0);

    // Determines if a contract initialization destination is required.
    ASTPointer<VariableDeclaration> dest;
    auto rvs = FUNC.returnParameters();
    if (!rvs.empty() && m_stack->allocations()->retval_is_allocated(*rvs[0]))
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
            rvs, FUNC.parameters(), &USER, dest, CONTEXT
        );

        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            FunctionBlockConverter cov(FUNC, m_stack);
            cov.set_for(_spec);
            body = cov.convert();
        }

        string base_fname = _spec.name(mods.len());
        auto id = make_shared<CVarDecl>(_rv_type, move(base_fname), _rv_is_ptr);
        defs.emplace_back(id, move(params), move(body));
    }

    // Generates a declaration for each modifier.
    CParams const mod_params = generate_params(
        rvs, FUNC.parameters(), &USER, dest, CONTEXT, true
    );
    for (size_t i = mods.len(); i > 0; --i)
    {
        size_t const IDX = i - 1;

        shared_ptr<CBlock> body;
        if (!M_FWD_DCL)
        {
            body = mods.generate(IDX, m_stack).convert();
        }

        auto id = make_shared<CVarDecl>(_rv_type, _spec.name(IDX), _rv_is_ptr);
        defs.emplace_back(id, mod_params, move(body));
    }

    // Prints each declaration.
    for (auto const& def : defs)
    {
        (*m_ostream) << def;
    }

    return _spec.name(0);
}

// --------------------------------------------------------------------------

}
}
}
