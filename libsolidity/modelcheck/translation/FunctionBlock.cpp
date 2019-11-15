/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code. This code
 * handles cases specific to function blocks.
 */

#include <libsolidity/modelcheck/translation/Block.h>

#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/translation/Expression.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/Types.h>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

FunctionBlockConverter::FunctionBlockConverter(
    FunctionDefinition const& _func, TypeConverter const& _types
): GeneralBlockConverter(
	_func.parameters(),
	_func.body(),
	_types,
	_func.modifiers().empty(),
	_func.isPayable()
), M_TYPES(_types)
{
	// TODO(scottwe): support multiple return types.
	if (_func.returnParameters().size() > 1)
	{
		throw runtime_error("Multiple return values not yet supported.");
	}
	else if (!_func.returnParameters().empty())
	{
		m_rv = _func.returnParameters()[0];
	}
}

// -------------------------------------------------------------------------- //

void FunctionBlockConverter::enter(
    CBlockList & _stmts, VariableScopeResolver & _decls
)
{
    if (m_rv && !m_rv->name().empty())
    {
		_decls.record_declaration(*m_rv);
		_stmts.push_back(make_shared<CVarDecl>(
			M_TYPES.get_type(*m_rv), _decls.resolve_declaration(*m_rv)
        ));
    }
}

void FunctionBlockConverter::exit(
    CBlockList & _stmts, VariableScopeResolver & _decls
)
{
    if (m_rv && !m_rv->name().empty())
    {
		auto rv = make_shared<CVarDecl>(
			M_TYPES.get_type(*m_rv), _decls.resolve_declaration(*m_rv)
		);
		_stmts.push_back(make_shared<CReturn>(rv->id()));
    }
}

// -------------------------------------------------------------------------- //

bool FunctionBlockConverter::visit(Return const& _node)
{
	CExprPtr rv = nullptr;
	if (_node.expression())
	{
        rv = expand(*_node.expression());
		rv = FunctionUtilities::try_to_wrap(*m_rv->annotation().type, move(rv));
	}
    new_substmt<CReturn>(rv);
	return false;
}

// -------------------------------------------------------------------------- //

}
}
}
