#include <libsolidity/modelcheck/utils/AST.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ExpressionCleaner::ExpressionCleaner(Expression const& _expr)
{
    _expr.accept(*this);
}

Expression const& ExpressionCleaner::clean() const
{
    return (*dynamic_cast<Expression const*>(m_res));
}

bool ExpressionCleaner::visitNode(ASTNode const& _node)
{
    m_res = (&_node);
    return false;
}

bool ExpressionCleaner::visit(TupleExpression const& _node)
{
    return (_node.components().size() == 1);
}


// -------------------------------------------------------------------------- //

VariableDeclaration const* member_access_to_decl(MemberAccess const& _access)
{
	auto const EXPR_TYPE = _access.expression().annotation().type;
    if (auto contract_type = dynamic_cast<ContractType const*>(EXPR_TYPE))
    {
        for (auto member : contract_type->contractDefinition().stateVariables())
        {
            if (member->name() == _access.memberName())
            {
                return member;
            }
        }
    }
    else if (auto struct_type = dynamic_cast<StructType const*>(EXPR_TYPE))
	{
        for (auto member : struct_type->structDefinition().members())
        {
            if (member->name() == _access.memberName())
            {
                return member.get();
            }
        }
	}
    return nullptr;
}

VariableDeclaration const* expr_to_decl(Expression const& _expr)
{
    auto const& top_expr = ExpressionCleaner(_expr).clean();
    if (auto const* member = dynamic_cast<MemberAccess const*>(&top_expr))
    {
        return member_access_to_decl(*member);
    }
    else if (auto const* id = dynamic_cast<Identifier const*>(&top_expr))
    {
        auto raw = id->annotation().referencedDeclaration;
        return dynamic_cast<VariableDeclaration const*>(raw);
    }
    throw runtime_error("Could not convert expression to decl.");
}

// -------------------------------------------------------------------------- //

}
}
}
