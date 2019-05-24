/*
 * TODO
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <libsolidity/modelcheck/MapDepthCalculator.h>
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

#define SOL_TCAST(Typename, ptr) dynamic_cast<Typename const*>(ptr)

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

Translation TypeTranslator::translate(ContractDefinition const& datatype) const
{
    Translation t;
    t.name = datatype.name();
    t.type = "struct " + t.name;
    return t;
}

Translation TypeTranslator::translate(StructDefinition const& datatype) const
{
    if (!m_contract_ctx.is_initialized())
    {
        throw runtime_error("A struct must be translated within a contract.");
    }

    Translation t;
    t.name = m_contract_ctx.value() + "_" + datatype.name();
    t.type = "struct " + t.name;
    return t;
}

Translation TypeTranslator::translate(Mapping const& datatype) const
{
    if (!m_map_ctx.is_initialized())
    {
        throw runtime_error("A map must be translated within some scope.");
    }

    MapDepthCalculator depth_calc(datatype);

    Translation t;
    t.name = scope().name + "_submap" + std::to_string(depth_calc.depth());
    t.type = "struct " + t.name;
    return t;
}

Translation TypeTranslator::translate(TypeName const& datatype) const
{
    Type const* t = datatype.annotation().type;

    Translation res;
    if ((SOL_TCAST(AddressType, t) != nullptr) ||
        (SOL_TCAST(StringLiteralType, t) != nullptr) ||
        (SOL_TCAST(BoolType, t) != nullptr))
    {
        res.type = res.name = "int";
    }
    else if (SOL_TCAST(IntegerType, t) != nullptr)
    {
        if (SOL_TCAST(IntegerType, t)->isSigned())
        {
            res.type = res.name = "int";
        }
        else
        {
            res.type = res.name = "unsigned int";
        }
    }
    else if ((SOL_TCAST(FixedPointType, t) != nullptr) ||
             (SOL_TCAST(RationalNumberType, t) != nullptr))
    {
        res.type = res.name = "double";
    }
    else if (SOL_TCAST(MappingType, t) != nullptr)
    {
        res = translate(datatype);
    }
    else if (SOL_TCAST(ContractType, t) != nullptr)
    {
        res = translate(SOL_TCAST(ContractType, t)->contractDefinition());
    }
    else if (SOL_TCAST(StructType, t) != nullptr)
    {
        res = translate(SOL_TCAST(StructType, t)->structDefinition());
    }
    else
    {
        // TODO: Add support for...
        //       - ReferenceType
        //       - ArrayType
        //       - EnumType
        //       - TupleType
        //       - FixedBytesType
        throw runtime_error(
            "Attempt to translate unsupported type:" + t->richIdentifier());
    }
    return res;
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

}
}
}
