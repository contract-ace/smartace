/**
 * @date 2019
 * A collection of mostly free functions, which allow for categorizing AST nodes
 * into C-struct groups. These are direct translations of the type analysis
 * specifications.
 */

#include <libsolidity/modelcheck/utils/Types.h>

#include <libsolidity/modelcheck/utils/General.h>
#include <map>
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

static const IntegerType G_ENUM_TYPE(8, IntegerType::Modifier::Unsigned);

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

    if (auto enum_type = dynamic_cast<EnumType const*>(type))
    {
        if (enum_type->numberOfMembers() > G_ENUM_TYPE.maxValue())
        {
            throw runtime_error("Enum has unsupported number of values.");
        }
        type = &G_ENUM_TYPE;
    }

    return *type;
}

// -------------------------------------------------------------------------- //

int simple_bit_count(Type const& _type)
{
    Type const& t = unwrap(_type);

    unsigned int raw_bits;
    switch(t.category())
    {
    case Type::Category::Address:
        return 160;
    case Type::Category::Bool:
        return 8;
    case Type::Category::FixedPoint:
        raw_bits = dynamic_cast<FixedPointType const&>(t).numBits();
        break;
    case Type::Category::Integer:
        raw_bits = dynamic_cast<IntegerType const&>(t).numBits();
        break;
    default:
        return 64;
    }

    return raw_bits;
}

// -------------------------------------------------------------------------- //

bool simple_is_signed(Type const& _type)
{
    Type const& t = unwrap(_type);

    switch(t.category())
    {
    case Type::Category::FixedPoint:
        return  dynamic_cast<FixedPointType const&>(t).isSigned();
    case Type::Category::Integer:
        return dynamic_cast<IntegerType const&>(t).isSigned();
    default:
        return false;
    }
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
    case Type::Category::Integer:
        return true;
    default:
        return false;
    }
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

string escape_decl_name_string(string const& _name)
{
    ostringstream oss;
    for (char const& c : _name)
    {
        oss << c;
        if (c == '_') oss << '_'; 
    }
    return oss.str();
}

string escape_decl_name(Declaration const& _decl)
{
    return escape_decl_name_string(_decl.name());
}

// -------------------------------------------------------------------------- //

}
}
}
