#include <libsolidity/modelcheck/model/Block.h>

#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
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
    FunctionSpecialization const& _spec
): M_SPEC(_spec)
{
    for (auto mod : _spec.func().modifiers())
    {
        auto contract = &_spec.source();
        string const& target = mod->name()->name();

        if (auto match = find_named_match<ModifierDefinition>(contract, target))
        {
            m_filtered_mods.push_back(make_pair(match, mod.get()));
        }
    }
}

// -------------------------------------------------------------------------- //

ModifierBlockConverter ModifierBlockConverter::Factory::generate(
    size_t _i,
    TypeAnalyzer const& _converter,
    CallState const& _statedata,
    AllocationGraph const& _alloc_graph
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
        _converter,
        _statedata,
        _alloc_graph,
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
    TypeAnalyzer const& _converter,
    CallState const& _statedata,
    AllocationGraph const& _alloc_graph,
    string _next,
    bool _entry
): GeneralBlockConverter(
    _def->parameters(),
    _func.returnParameters(),
    _def->body(),
    _converter,
    _statedata,
    _alloc_graph,
    _entry,
    _func.isPayable()
), M_STATEDATA(_statedata)
 , M_TRUE_PARAMS(_func.parameters())
 , M_USER_PARAMS(_def->parameters())
 , M_USER_ARGS(_curr->arguments())
 , M_NEXT_CALL(move(_next))
 , m_shadow_decls(CodeType::SHADOWBLOCK)
{
	if (has_retval())
	{
	    // TODO(scottwe): support multiple return types.
        auto const& ARG = *_func.returnParameters()[0];
		m_rv = make_shared<CVarDecl>(
            M_CONVERTER.get_type(ARG),
            VariableScopeResolver::rewrite("rv", true, VarContext::FUNCTION)
        );
	}
    m_shadow_decls.enter();
    for (auto ARG : M_TRUE_PARAMS)
    {
        m_shadow_decls.record_declaration(*ARG);
    }
}


// -------------------------------------------------------------------------- //

void ModifierBlockConverter::enter(
    CBlockList & _stmts, VariableScopeResolver &_decls
)
{
    if (has_retval())
    {
		_stmts.push_back(m_rv);
    }

    if (M_USER_ARGS)
    {
        for (unsigned int i = 0; i < M_USER_ARGS->size(); ++i)
        {
            auto const& PARAM = *M_USER_PARAMS[i];
            auto const& ARG = *((*M_USER_ARGS)[i]);

            auto const TYPE = M_CONVERTER.get_type(PARAM);
            auto const SYM = _decls.resolve_declaration(PARAM);

            bool const IS_INIT = block_type() == BlockType::Initializer;

            ExpressionConverter arg_converter(
                ARG, M_CONVERTER, M_STATEDATA, m_shadow_decls, false, IS_INIT
            );

            auto expr = arg_converter.convert();
            expr = InitFunction::wrap(*PARAM.type(), move(expr));
            _stmts.push_back(make_shared<CVarDecl>(TYPE, SYM, false, expr));
        }
    }
}

void ModifierBlockConverter::exit(CBlockList & _stmts, VariableScopeResolver &)
{
    if (has_retval())
    {
        _stmts.push_back(make_shared<CReturn>(m_rv->id()));
    }
}

// -------------------------------------------------------------------------- //

bool ModifierBlockConverter::visit(Return const&)
{
    CExprPtr rv_id = nullptr;
    if (has_retval())
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
	M_STATEDATA.compute_next_state_for(builder, false, nullptr);

	for (auto const& ARG : M_TRUE_PARAMS)
	{
        auto const SYM = m_shadow_decls.resolve_declaration(*ARG);
		builder.push(make_shared<CIdentifier>(SYM, false));
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
