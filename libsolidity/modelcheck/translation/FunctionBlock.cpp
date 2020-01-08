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
    FunctionDefinition const& _func,
	CallState const& _statedata,
	TypeConverter const& _types
): GeneralBlockConverter(
	_func.parameters(),
	_func.returnParameters(),
	_func.body(),
	_statedata,
	_types,
	_func.modifiers().empty(),
	_func.isPayable()
), M_TYPES(_types)
{
	if (block_type() == BlockType::Operation)
	{
		// TODO(scottwe): support multiple return types.
		m_rv = _func.returnParameters()[0];
	}
}

// -------------------------------------------------------------------------- //

void FunctionBlockConverter::set_for(FunctionSpecialization const& _for)
{
	m_spec = &_for;
}

// -------------------------------------------------------------------------- //

void FunctionBlockConverter::enter(
    CBlockList & _stmts, VariableScopeResolver & _decls
)
{
	_decls.assign_spec(m_spec);
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
	CExprPtr rv;
	switch (block_type())
	{
	case BlockType::Action:
		new_substmt<CReturn>(nullptr);
		break;
	case BlockType::Operation:
		rv = expand(*_node.expression());
		rv = FunctionUtilities::try_to_wrap(*m_rv->annotation().type, move(rv));
		new_substmt<CReturn>(rv);
		break;
	case BlockType::Initializer:
		new_substmt<CExprStmt>(expand(*_node.expression()));
		break;
	}
	return false;
}

// -------------------------------------------------------------------------- //

}
}
}
