/**
 * @date 2019
 * A collection of mostly free functions, which allow for categorizing AST nodes
 * into C-struct groups. These are direct translations of the type analysis
 * specifications.
 */

#include <libsolidity/modelcheck/TypeClassification.h>
#include <libsolidity/modelcheck/Utility.h>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

Type const& unwrap(Type const& _type)
{
    Type const* type = &_type;

    while (auto type_type = dynamic_cast<TypeType const*>(type))
    {
        type = type_type->actualType();
    }

    if (auto rat_type = dynamic_cast<RationalNumberType const*>(type))
    {
        if (rat_type->isFractional()) type = rat_type->fixedPointType();
        else type = rat_type->integerType();
    }

    return *type;
}

// -------------------------------------------------------------------------- //

bool is_wrapped_type(Type const& _type)
{
    Type const& t = unwrap(_type);

    switch(t.category())
    {
    case Type::Category::Address:
    case Type::Category::Bool:
    case Type::Category::FixedPoint:
        return true;
    case Type::Category::Integer:
    {
        unsigned int const BITS = dynamic_cast<IntegerType const&>(t).numBits();
        return (BITS > 64) || (!is_power_of_two(BITS));
    }
    default:
        return false;
    }
}

// -------------------------------------------------------------------------- //

bool has_wrapped_data(Declaration const& _expr)
{
    return is_wrapped_type(*_expr.type());
}

bool has_wrapped_data(TypeName const& _expr)
{
    return is_wrapped_type(*_expr.annotation().type);
}

bool has_wrapped_data(Expression const& _expr)
{
    return is_wrapped_type(*_expr.annotation().type);
}

// -------------------------------------------------------------------------- //

bool is_simple_type(Type const& _type)
{
    Type const& type = unwrap(_type);
    switch (type.category())
    {
    case Type::Category::Address:
    case Type::Category::Integer:
    case Type::Category::RationalNumber:
    case Type::Category::Bool:
    case Type::Category::FixedPoint:
    case Type::Category::Enum:
        return true;
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

string escape_decl_name(Declaration const& _decl)
{
    ostringstream oss;
    for (char const& c : _decl.name())
    {
        oss << c;
        if (c == '_') oss << '_'; 
    }
    return oss.str();
}

// -------------------------------------------------------------------------- //

}
}
}
