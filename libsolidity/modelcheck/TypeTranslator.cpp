/*
 * TODO
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <sstream>
#include <stdexcept>

using namespace std;
using namespace boost;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

void TypeTranslator::enter_scope(ContractDefinition const& scope)
{
    if (m_contract_ctx.is_initialized())
    {
        throw runtime_error("Contracts are not nestable.");
    }
    m_contract_ctx = scope.name();
}

void TypeTranslator::enter_scope(StructDefinition const& scope)
{
    if (!m_contract_ctx.is_initialized())
    {
        throw runtime_error("Structs must be defined within contracts.");
    }
    if (m_struct_ctx.is_initialized())
    {
        throw runtime_error("Structs are not nestable.");
    }
    m_struct_ctx = scope.name();
}

void TypeTranslator::enter_scope(VariableDeclaration const& scope)
{
    if (!m_contract_ctx.is_initialized())
    {
        throw runtime_error("VariableDeclarations must be nested in a type.");
    }
    m_map_ctx = scope.name();
}

void TypeTranslator::exit_scope()
{
    if (m_map_ctx.is_initialized())
    {
        m_map_ctx.reset();
    }
    else if (m_struct_ctx.is_initialized())
    {
        m_struct_ctx.reset();
    }
    else if (m_contract_ctx.is_initialized())
    {
        m_contract_ctx.reset();
    }
    else
    {
        throw runtime_error("Translator out of scope.");
    }
}

Translation TypeTranslator::translate(Declaration const& datatype) const
{
    return translate(datatype.type());
}

Translation TypeTranslator::translate(TypeName const& datatype) const
{
    return translate(datatype.annotation().type);
}

Translation TypeTranslator::translate(TypePointer t) const
{
    if (!t)
    {
        throw runtime_error("nullptr provided to type translator.");
    }

    switch (t->category())
    {
    case Type::Category::Address:
    case Type::Category::Bool:
    case Type::Category::StringLiteral:
        return translate_basic("int");
    case Type::Category::Integer:
        return translate_int(t);
    case Type::Category::RationalNumber:
    case Type::Category::FixedPoint:
        return translate_basic("double");
    case Type::Category::Array:
    case Type::Category::FixedBytes:
        throw runtime_error("Array types are not supported");
    case Type::Category::Contract:
        return translate_contract(t);
    case Type::Category::Struct:
        return translate_struct(t);
    case Type::Category::Function:
        throw runtime_error("Function types are not supported.");
    case Type::Category::Enum:
        throw runtime_error("Enum types are not supported.");
    case Type::Category::Tuple:
        throw runtime_error("Tuple types are not supported.");
    case Type::Category::Mapping:
        return translate_mapping(t);
    case Type::Category::TypeType:
        return translate_type(t);
    case Type::Category::Modifier:
        throw runtime_error("Modifier types are not supported.");
    case Type::Category::Magic:
        throw runtime_error("Magic types are not supported.");
    case Type::Category::Module:
        throw runtime_error("Module types are not supported.");
	case Type::Category::InaccessibleDynamic:
        throw runtime_error("Type is inaccessible.");
    default:
        throw runtime_error("Unknown type:" + t->richIdentifier());
    }
}

Translation TypeTranslator::scope() const
{
    if (!m_contract_ctx.is_initialized())
    {
        throw runtime_error("No contract is currently in scope.");
    }

    Translation t;
    t.name = m_contract_ctx.value();
    if (m_struct_ctx.is_initialized())
    {
        t.name += ("_" + m_struct_ctx.value());
    }
    if (m_map_ctx.is_initialized())
    {
        t.name += ("_" + m_map_ctx.value());
    }
    t.type = "struct " + t.name;
    return t;
}

Translation TypeTranslator::translate_basic(const string &name) const
{
    Translation res;
    res.type = res.name = name;
    return res;
}

Translation TypeTranslator::translate_int(TypePointer t) const
{
    auto typecast = dynamic_cast<IntegerType const*>(t);
    if (!typecast)
    {
        throw runtime_error("Type not convertible to IntegerType");
    }

    Translation res;
    res.type = res.name = typecast->isSigned() ? "int" : "unsigned int";
    return res;
}

Translation TypeTranslator::translate_contract(TypePointer t) const
{
    auto typecast = dynamic_cast<ContractType const*>(t);
    if (!typecast)
    {
        throw runtime_error("Type not convertible to ContractType");
    }

    Translation res;
    res.name = typecast->contractDefinition().name();
    res.type = "struct " + res.name;
    return res;
}

Translation TypeTranslator::translate_struct(TypePointer t) const
{
    auto typecast = dynamic_cast<StructType const*>(t);
    if (!typecast)
    {
        throw runtime_error("Type not convertible to StructType");
    }

    auto const& str_decl = typecast->structDefinition();
    auto const& ctx_decl = dynamic_cast<Declaration const*>(str_decl.scope());
    if (!ctx_decl)
    {
        throw runtime_error("Expected Structure definition within Declaration.");
    }

    Translation res;
    res.name  = ctx_decl->name() + "_" + str_decl.name();
    res.type = "struct " + res.name;
    return res;
}

Translation TypeTranslator::translate_mapping(TypePointer t) const
{
    auto typecast = dynamic_cast<MappingType const*>(t);
    if (!typecast)
    {
        throw runtime_error("Type not convertible to MappingType");
    }

    unsigned int depth = 0;
    MappingType const* curr_type = typecast;
    do {
        ++depth;
        curr_type = dynamic_cast<MappingType const*>(curr_type->valueType());
    } while (curr_type != nullptr);

    if (!m_map_ctx.is_initialized())
    {
        throw runtime_error("A map must be translated within some scope.");
    }

    Translation res;
    res.name = scope().name + "_submap" + std::to_string(depth);
    res.type = "struct " + res.name;
    return res;
}

Translation TypeTranslator::translate_type(TypePointer t) const
{
    auto typecast = dynamic_cast<TypeType const*>(t);
    if (!typecast)
    {
        throw runtime_error("Type not convertible to TypeType");
    }
    return translate(typecast->actualType());
}

}
}
}
