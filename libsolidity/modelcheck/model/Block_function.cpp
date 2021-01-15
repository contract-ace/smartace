#include <libsolidity/modelcheck/model/Block.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/model/Expression.h>
#include <libsolidity/modelcheck/utils/AST.h>
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
    FunctionDefinition const& _func, shared_ptr<AnalysisStack const> _stack
): GeneralBlockConverter(
	_func.parameters(),
	_func.returnParameters(),
	_func.body(),
	_stack,
	_func.modifiers().empty(),
	_func.isPayable()
), m_rvs(_func.returnParameters()) {}

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
	for (size_t i = 0; i < m_rvs.size(); ++i)
	{
		auto rv = m_rvs[i];
		bool const IS_UNNAMED_RV = rv->name().empty();

		string rv_name;
		if (IS_UNNAMED_RV)
		{
			rv_name = VariableScopeResolver::rewrite(
				to_string(i), false, VarContext::FUNCTION
			);
		}
		else
		{
			_decls.record_declaration(*rv);
			rv_name = _decls.resolve_declaration(*rv);
		}

		string const RV_TYPE = m_stack->types()->get_type(*rv);
		auto const RV_INIT = m_stack->types()->get_init_val(*rv);
		bool const IS_REF = decl_is_ref(*rv);
		
		auto decl = make_shared<CVarDecl>(RV_TYPE, rv_name, IS_REF, RV_INIT);
		m_rv_decls.push_back(decl);
		if (i > 0)
		{
			auto deref_var = make_shared<CDereference>(decl->id());
			auto assign_expr = make_shared<CBinaryOp>(deref_var, "=", RV_INIT);
			_stmts.push_back(assign_expr->stmt());
		}
		else if (!IS_UNNAMED_RV)
		{
			_stmts.push_back(decl);
		}
	}
}

void FunctionBlockConverter::exit(CBlockList & _stmts, VariableScopeResolver &)
{
    if (!m_rvs.empty() && (!m_rvs[0]->name().empty()))
    {
		_stmts.push_back(make_shared<CReturn>(m_rv_decls[0]->id()));
    }
}

// -------------------------------------------------------------------------- //

bool FunctionBlockConverter::visit(Return const& _node)
{
	switch (block_type())
	{
	case BlockType::Action:
		new_substmt<CReturn>(nullptr);
		break;
	case BlockType::Operation:
		{
			vector<Expression const*> expr_list;
			if (m_rvs.size() == 1)
			{
				expr_list.push_back(_node.expression());
			}
			else
			{
				ExpressionCleaner cleaner(*_node.expression());
				auto raw = &cleaner.clean();
				auto tuple = dynamic_cast<TupleExpression const*>(raw);
				for (auto val : tuple->components())
				{
					expr_list.push_back(val.get());
				}
			}

			CBlockList stmts;
			for (size_t i = expr_list.size(); i > 0; --i)
			{
				CExprPtr val = expand(*expr_list[i - 1]);
				val = InitFunction::wrap(*m_rvs[i - 1]->type(), move(val));
				if (m_rvs[i - 1]->type()->category() == Type::Category::Contract)
				{
					val = make_shared<CReference>(val);
				}

				if (i == 1)
				{
					stmts.push_back(make_shared<CReturn>(val));
				}
				else
				{
					auto dest = make_shared<CDereference>(m_rv_decls[i - 1]->id());
					auto op = make_shared<CBinaryOp>(dest, "=", val);
					stmts.push_back(op->stmt());
				}
			}

			new_substmt<CBlock>(stmts);
			break;
		}
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
