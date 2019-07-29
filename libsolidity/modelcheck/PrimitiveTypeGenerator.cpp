/**
 * @date 2019
 * Visitor used to generate all primitive type declarations. This will produce
 * a header-only library for arithmetic operations.
 */

#include <libsolidity/modelcheck/PrimitiveTypeGenerator.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

PrimitiveTypeGenerator::PrimitiveTypeGenerator(ASTNode const& _root)
: M_SUMMARY(_root) {}

// -------------------------------------------------------------------------- //

bool PrimitiveTypeGenerator::found_bool()
{
    return M_SUMMARY.m_uses_bool;
}

bool PrimitiveTypeGenerator::found_address()
{
    return M_SUMMARY.m_uses_address;
}

bool PrimitiveTypeGenerator::found_int(unsigned char _bytes)
{
    return M_SUMMARY.m_uses_int[_bytes - 1];
}

bool PrimitiveTypeGenerator::found_uint(unsigned char _bytes)
{
    return M_SUMMARY.m_uses_uint[_bytes - 1];
}

bool PrimitiveTypeGenerator::found_fixed(unsigned char _bytes, unsigned char _d)
{
    return M_SUMMARY.m_uses_fixed[_bytes - 1][_d];
}

bool PrimitiveTypeGenerator::found_ufixed(unsigned char _bytes, unsigned char _d)
{
    return M_SUMMARY.m_uses_ufixed[_bytes - 1][_d];
}

// -------------------------------------------------------------------------- //

PrimitiveTypeGenerator::Visitor::Visitor(ASTNode const& _root) {
    for (unsigned char bytes = 0; bytes < 32; ++bytes)
    {
        m_uses_int[bytes] = false;
        m_uses_uint[bytes] = false;
        for (unsigned char fixed_pt = 0; fixed_pt <= 80; ++fixed_pt)
        {
            m_uses_fixed[bytes][fixed_pt] = false;
            m_uses_ufixed[bytes][fixed_pt] = false;
        }
    }
    _root.accept(*this);
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::Visitor::endVisit(UsingForDirective const& _node)
{
    process_type(_node.typeName()->annotation().type);
}

void PrimitiveTypeGenerator::Visitor::endVisit(VariableDeclaration const& _node)
{
    process_type(_node.type());
}

void PrimitiveTypeGenerator::Visitor::endVisit(ElementaryTypeName const& _node)
{
    process_type(_node.annotation().type);
}

void PrimitiveTypeGenerator::Visitor::endVisit(
    ElementaryTypeNameExpression const& _node
)
{
    process_type(_node.annotation().type);
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::Visitor::process_type(Type const* _type)
{
    switch (_type->category())
    {
    case Type::Category::Bool:
        m_uses_bool = true;
        break;
    case Type::Category::Address:
        m_uses_address = true;
        break;
    case Type::Category::Integer:
        if (auto int_type = dynamic_cast<IntegerType const*>(_type))
        {
            unsigned int bit_width = (int_type->numBits() / 8) - 1;
            if (int_type->isSigned())
            {
                m_uses_int[bit_width] = true;
            }
            else
            {
                m_uses_uint[bit_width] = true;
            }
        }
        else
        {
            throw runtime_error("Mismatched type category.");
        }
        break;
    case Type::Category::FixedPoint:
        if (auto fixed_type = dynamic_cast<FixedPointType const*>(_type))
        {
            unsigned int bit_width = (fixed_type->numBits() / 8) - 1;
            unsigned int dcm_width = fixed_type->fractionalDigits();
            if (fixed_type->isSigned())
            {
                m_uses_fixed[bit_width][dcm_width] = true;
            }
            else
            {
                m_uses_ufixed[bit_width][dcm_width] = true;
            }
        }
        else
        {
            throw runtime_error("Mismatched type category.");
        }
        break;
    default:
        break;
    }
}

// -------------------------------------------------------------------------- //

}
}
}
