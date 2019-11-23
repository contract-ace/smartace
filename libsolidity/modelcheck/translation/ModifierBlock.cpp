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

ModifierBlockConverter::Context::Context(
    FunctionDefinition const& _func, size_t _i
): func(_func)
{
    // Checks that _i is in bounds.
    if (func.modifiers().size() < _i)
    {
        throw runtime_error("FunctionDefinition of unknown i.");
    }

    // Resolves the contract on which the function resides.
    auto const* scope = dynamic_cast<ContractDefinition const*>(func.scope());
    if (!scope)
    {
        throw runtime_error("FunctionDefinition of unknown scope.");
    }

    // Extracts the inovcation and definition.
    curr = func.modifiers()[_i].get();
    for (auto const& mod : scope->functionModifiers())
    {
        if (mod->name() == curr->name()->name())
        {
            def = mod;
            break;
        }
    }
    if (!def)
    {
        throw runtime_error("FunctionDefinition has unknown modifier.");
    }

    // Analyzes the "next call". next_is_stateful defualts to true for case 1.
    if (_i + 1 < func.modifiers().size())
    {
        next = func.modifiers()[_i + 1].get();
    }
    else
    {
        next = &func;
    }

    // Determines payment behaviour.
    manage_pay = (_i == 0);
    is_payable = _func.isPayable();
}

// -------------------------------------------------------------------------- //

ModifierBlockConverter::ModifierBlockConverter(
    FunctionDefinition const& _func,
    size_t _i,
	CallState const& _statedata,
    TypeConverter const& _types
): ModifierBlockConverter(Context(_func, _i), _statedata, _types)
{
}

ModifierBlockConverter::ModifierBlockConverter(
    Context const& _ctx,
    CallState const& _statedata,
    TypeConverter const& _types
): GeneralBlockConverter(
    _ctx.def->parameters(),
    _ctx.def->body(),
    _statedata,
    _types,
    _ctx.manage_pay,
    _ctx.is_payable
), M_STATEDATA(_statedata)
 , M_TYPES(_types)
 , M_TRUE_PARAMS(_ctx.func.parameters())
 , M_USER_PARAMS(_ctx.def->parameters())
 , M_USER_ARGS(_ctx.curr->arguments())
 , M_NEXT_CALL(_types.get_name(*_ctx.next))
 , m_shadow_decls(true)
{
	// TODO(scottwe): support multiple return types.
	if (_ctx.func.returnParameters().size() > 1)
	{
		throw runtime_error("Multiple return values not yet supported.");
	}
	else if (!_ctx.func.returnParameters().empty())
	{
        auto const& ARG = *_ctx.func.returnParameters()[0];
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
	M_STATEDATA.push_state_to(builder);

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
