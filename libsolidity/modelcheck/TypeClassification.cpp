/**
 * @date 2019
 * A collection of mostly free functions, which allow for categorizing AST nodes
 * into C-struct groups. These are direct translations of the type analysis
 * specifications.
 */

#include <libsolidity/modelcheck/TypeClassification.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

bool is_simple_type(Type const& _type)
{
    switch (_type.category())
    {
        case Type::Category::Address:
        case Type::Category::Integer:
        case Type::Category::RationalNumber:
        case Type::Category::Bool:
        case Type::Category::FixedPoint:
        case Type::Category::Enum:
            return true;
        case Type::Category::TypeType:
            return is_simple_type(
                *dynamic_cast<TypeType const&>(_type).actualType()
            );
        default:
            return false;
    }
}

// -------------------------------------------------------------------------- //

bool has_simple_type(Declaration const& _node)
{
    return is_simple_type(*_node.type());
}

bool has_simple_type(TypeName const& _node)
{
    return is_simple_type(*_node.annotation().type);
}

bool has_simple_type(Expression const& _node)
{
    return is_simple_type(*_node.annotation().type);
}

// -------------------------------------------------------------------------- //

}
}
}
