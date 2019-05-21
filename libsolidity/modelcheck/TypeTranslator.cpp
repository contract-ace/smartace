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

void TypeTranslator::enter_scope(const ContractDefinition &scope)
{
    if (m_contract_ctx.is_initialized())
    {
        throw runtime_error("Contracts are not nestable.");
    }
    m_contract_ctx = scope.name();
}

void TypeTranslator::enter_scope(const StructDefinition &scope)
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

void TypeTranslator::enter_scope(const VariableDeclaration &scope)
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

std::string TypeTranslator::translate(const ContractDefinition &datatype) const
{
    ostringstream oss;
    oss << "struct " << datatype.name();
    return oss.str();
}

std::string TypeTranslator::translate(const StructDefinition &datatype) const
{
    if (!m_contract_ctx.is_initialized())
    {
        throw runtime_error("A struct must be translated within a contract.");
    }

    ostringstream oss;
    oss << "struct " << m_contract_ctx.value() << "_" << datatype.name();
    return oss.str();
}

std::string TypeTranslator::translate(const Mapping &datatype) const
{
    if (!m_map_ctx.is_initialized())
    {
        throw runtime_error("A map must be translated within some scope.");
    }

    ostringstream oss;
    MapDepthCalculator depth_calc(datatype);

    oss << "struct" << " " << m_contract_ctx.value() << "_";
    if (m_struct_ctx.is_initialized())
    {
        oss << m_struct_ctx.value() << "_";
    }
    oss << m_map_ctx.value() << "_submap" << depth_calc.depth();
    return oss.str();
}

}
}
}
