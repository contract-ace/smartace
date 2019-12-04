/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code. This code
 * handles cases specific to function blocks.
 */

#include <libsolidity/modelcheck/translation/Block.h>

#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/translation/Expression.h>
#include <libsolidity/modelcheck/utils/Function.h>
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
    FunctionDefinition const& _func, string _name
): m_func(_func), m_name(move(_name))
{
    auto const* scope = dynamic_cast<ContractDefinition const*>(_func.scope());
    if (!scope)
    {
        throw runtime_error("FunctionDefinition of unknown scope.");
    }

    auto const& reference_modifiers = scope->functionModifiers();
    for (auto mod : _func.modifiers())
    {
        for (auto const* ref : reference_modifiers)
        {
            if (mod->name()->name() == ref->name())
            {
                m_filtered_mods.push_back(make_pair(ref, mod.get()));
                break;
            }
        }
    }
}

// -------------------------------------------------------------------------- //

ModifierBlockConverter ModifierBlockConverter::Factory::generate(
    size_t _i, CallState const& _statedata, TypeConverter const& _types
)
{
    // Checks that _i is in bounds.
    if (len() < _i)
    {
        throw runtime_error("FunctionDefinition of unknown i.");
    }

    // Finds the definition/invocation pair.
    auto const& def_invoke_pair = m_filtered_mods[_i];

    // Computes the next call name.
    string next_name;
    if (_i + 1 < len())
    {
        next_name = FunctionUtilities::modifier_name(m_name, _i + 1);
    }
    else
    {
        next_name = FunctionUtilities::base_name(m_name);
    }

    // Constructors the bodifier block.
    return ModifierBlockConverter(
        m_func,
        def_invoke_pair.first,
        def_invoke_pair.second,
        _statedata,
        _types,
        next_name,
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
    CallState const& _statedata,
    TypeConverter const& _types,
    std::string _next,
    bool _entry
): GeneralBlockConverter(
    _def->parameters(),
    _def->body(),
    _statedata,
    _types,
    _entry,
    _func.isPayable()
), M_STATEDATA(_statedata)
 , M_TYPES(_types)
 , M_TRUE_PARAMS(_func.parameters())
 , M_USER_PARAMS(_def->parameters())
 , M_USER_ARGS(_curr->arguments())
 , M_NEXT_CALL(move(_next))
 , m_shadow_decls(CodeType::SHADOWBLOCK)
{
	// TODO(scottwe): support multiple return types.
	if (_func.returnParameters().size() > 1)
	{
		throw runtime_error("Multiple return values not yet supported.");
	}
	else if (!_func.returnParameters().empty())
	{
        auto const& ARG = *_func.returnParameters()[0];
		m_rv = make_shared<CVarDecl>(
            M_TYPES.get_type(ARG),
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

            auto const TYPE = M_TYPES.get_type(PARAM);
            auto const SYM = _decls.resolve_declaration(PARAM);

            ExpressionConverter arg_converter(
                ARG, M_STATEDATA, M_TYPES, m_shadow_decls
            );

            auto expr = arg_converter.convert();
            expr = FunctionUtilities::try_to_wrap(*PARAM.type(), move(expr));
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
	M_STATEDATA.compute_next_state_for(builder, false, nullptr);

	for (auto const& ARG : M_TRUE_PARAMS)
	{
        auto const SYM = m_shadow_decls.resolve_declaration(*ARG);
		builder.push(make_shared<CIdentifier>(SYM, false));
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
