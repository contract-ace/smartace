#include <libsolidity/modelcheck/model/Block.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/model/Expression.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/Types.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ModifierBlockConverter::ModifierBlockConverter::Factory::Factory(
    shared_ptr<AnalysisStack const> _stack,
    FunctionSpecialization const& _spec
): M_SPEC(_spec)
{
    auto contract = _stack->model()->get(_spec.use_by());

    for (auto mod : _spec.func().modifiers())
    {
        string const& target = mod->name()->name();

        for (auto match : contract->modifiers())
        {
            if (match->name() == target)
            {
                m_filtered_mods.push_back(make_pair(match, mod.get()));
            }
        }
    }
}

// -------------------------------------------------------------------------- //

ModifierBlockConverter ModifierBlockConverter::Factory::generate(
    size_t _i, shared_ptr<AnalysisStack const> _stack
) const
{
    // Checks that _i is in bounds.
    if (len() < _i)
    {
        throw runtime_error("FunctionDefinition of unknown i.");
    }

    // Finds the definition/invocation pair.
    auto const& def_invoke_pair = m_filtered_mods[_i];

    // Constructors the modifier block.
    return ModifierBlockConverter(
        M_SPEC.func(),
        def_invoke_pair.first,
        def_invoke_pair.second,
        _stack,
        M_SPEC.name(_i + 1),
        _i == 0
    );
}

// --------------------------------------------------------------------------

size_t ModifierBlockConverter::Factory::len() const
{
    return m_filtered_mods.size();
}

// -------------------------------------------------------------------------- //

bool ModifierBlockConverter::Factory::empty() const
{
    return m_filtered_mods.empty();
}

// -------------------------------------------------------------------------- //

ModifierBlockConverter::ModifierBlockConverter(
    FunctionDefinition const& _func,
    ModifierDefinition const* _def,
    ModifierInvocation const* _curr,
    shared_ptr<AnalysisStack const> _stack,
    string _next,
    bool _entry
): GeneralBlockConverter(
    _def->parameters(),
    _func.returnParameters(),
    _def->body(),
    _stack,
    _entry,
    _func.isPayable()
), M_TRUE_PARAMS(_func.parameters())
 , M_TRUE_RVS(_func.returnParameters())
 , M_USER_PARAMS(_def->parameters())
 , M_USER_ARGS(_curr->arguments())
 , M_NEXT_CALL(move(_next))
 , m_shadow_decls(CodeType::SHADOWBLOCK)
{
	if (has_retval())
	{
	    // TODO(scottwe): support multiple return types.
        auto rv = _func.returnParameters()[0];
        auto rv_type = m_stack->types()->get_type(*rv);
        auto rv_name = m_shadow_decls.rewrite("rv", true, VarContext::FUNCTION);
		m_rv = make_shared<CVarDecl>(rv_type, rv_name);
	}

    m_shadow_decls.enter();
    for (auto ARG : M_TRUE_PARAMS)
    {
        m_shadow_decls.record_declaration(*ARG);
    }
    for (auto RV : M_TRUE_RVS)
    {
        m_shadow_decls.record_declaration(*RV);
    }
}


// -------------------------------------------------------------------------- //

void ModifierBlockConverter::enter(
    CBlockList & _stmts, VariableScopeResolver &_decls
)
{
    if (m_rv)
    {
		_stmts.push_back(m_rv);
    }

    if (M_USER_ARGS)
    {
        for (unsigned int i = 0; i < M_USER_ARGS->size(); ++i)
        {
            auto const& PARAM = *M_USER_PARAMS[i];
            auto const& ARG = *((*M_USER_ARGS)[i]);

            auto const TYPE = m_stack->types()->get_type(PARAM);
            auto const SYM = _decls.resolve_declaration(PARAM);

            bool const INITS = block_type() == BlockType::Initializer;

            ExpressionConverter arg_converter(
                ARG, m_stack, m_shadow_decls, false, INITS
            );

            auto expr = arg_converter.convert();
            expr = InitFunction::wrap(*PARAM.type(), move(expr));
            _stmts.push_back(make_shared<CVarDecl>(TYPE, SYM, false, expr));
        }
    }
}

void ModifierBlockConverter::exit(CBlockList & _stmts, VariableScopeResolver &)
{
    if (m_rv)
    {
        _stmts.push_back(make_shared<CReturn>(m_rv->id()));
    }
}

// -------------------------------------------------------------------------- //

bool ModifierBlockConverter::visit(Return const&)
{
    CExprPtr rv_id = nullptr;
    if (m_rv)
    {
        rv_id = m_rv->id();
    }
    new_substmt<CReturn>(rv_id);
    return false;
}

// -------------------------------------------------------------------------- //

void ModifierBlockConverter::endVisit(PlaceholderStatement const&)
{
	CFuncCallBuilder builder(M_NEXT_CALL);
	builder.push(make_shared<CIdentifier>("self", true));
	m_stack->environment()->compute_next_state_for(
        builder, false, true, nullptr
    );

    for (size_t i = 1; i < M_TRUE_RVS.size(); ++i)
    {
        auto const& RV = M_TRUE_RVS[i];
        string name = m_shadow_decls.resolve_declaration(*RV);
		builder.push(make_shared<CIdentifier>(name, false));
    }

	for (size_t i = 0; i < M_TRUE_PARAMS.size(); ++i)
	{
        auto const& ARG = M_TRUE_PARAMS[i];

        string name;
        if (ARG->name().empty())
        {
            // TODO(scottwe): factor out.
            name = "var" + to_string(i);
            name = VariableScopeResolver::rewrite(name, true, VarContext::FUNCTION);
        }
        else
        {
            name = m_shadow_decls.resolve_declaration(*ARG);
        }

		builder.push(make_shared<CIdentifier>(name, false));
	}

    if (block_type() == BlockType::Initializer)
    {
        builder.push(make_shared<CIdentifier>(InitFunction::INIT_VAR, true));
    }

    CExprPtr call = builder.merge_and_pop();
    if (m_rv)
    {
        new_substmt<CExprStmt>(m_rv->id()->assign(call));
    }
    else
    {
        new_substmt<CExprStmt>(call);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
