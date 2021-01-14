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

string get_error_type(ASTNode const* _node)
{
    string name = "unknown";
    if (_node == nullptr)
    {
        name = "NULL_C_POINTER";
    }
    else if (auto DECL = dynamic_cast<Declaration const*>(_node))
    {
        name = DECL->name();
    }
    else if (auto EXPR = dynamic_cast<Expression const*>(_node))
    {
        name += "(" + EXPR->annotation().type->canonicalName() + ")";
    }
    else if (auto TYPE = dynamic_cast<TypeName const*>(_node))
    {
        name += "(" + TYPE->annotation().type->canonicalName() + ")";
    }
    return name;
}

// -------------------------------------------------------------------------- //

string get_ast_string(ASTNode const* _node)
{
	auto const& LOC = _node->location();
	auto const& SRC = LOC.source->source();
	string str = SRC.substr(LOC.start, LOC.end - LOC.start);
	str.erase(remove(str.begin(), str.end(), '\n'), str.end());
    return str;
}

// -------------------------------------------------------------------------- //

ExpressionCleaner::ExpressionCleaner(Expression const& _expr)
{
    m_res = (&_expr);
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

bool decl_is_ref(VariableDeclaration const& _decl)
{
    return (_decl.referenceLocation() == VariableDeclaration::Storage);
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

Declaration const* node_to_ref(ASTNode const& _node)
{
    if (auto raw = dynamic_cast<Expression const*>(&_node))
    {
        auto const& EXPR = ExpressionCleaner(*raw).clean();
        if (auto const* member = dynamic_cast<MemberAccess const*>(&EXPR))
        {
            return member_access_to_decl(*member);
        }
        else if (auto const* id = dynamic_cast<Identifier const*>(&EXPR))
        {
            return id->annotation().referencedDeclaration;
        }
    }
    else if (auto type = dynamic_cast<UserDefinedTypeName const*>(&_node))
    {
        return type->annotation().referencedDeclaration;
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
