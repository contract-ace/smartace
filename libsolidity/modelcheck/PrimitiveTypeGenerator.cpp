/**
 * @date 2019
 * Visitor used to generate all primitive type declarations. This will produce
 * a header-only library for arithmetic operations.
 */

#include <libsolidity/modelcheck/PrimitiveTypeGenerator.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/Utility.h>
#include <sstream>

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

void PrimitiveTypeGenerator::print(ostream& _out)
{
    _out << "#pragma once" << endl;
    _out << "#include <stdint.h>" << endl;
    if (found_bool())
    {
        CStructDef decl("bool", make_shared<CParams>(CParams{
            make_shared<CVarDecl>("uint8_t", "v")
        }));
        _out << decl << *decl.make_typedef("bool_t");
    }
    if (found_address())
    {
        CStructDef decl("address", make_shared<CParams>(CParams{
            make_shared<CVarDecl>("uint64_t", "v")
        }));
        _out << decl << *decl.make_typedef("address_t");
    }
    for (unsigned char bytes = 1; bytes <= 32; ++bytes)
    {
        if (found_int(bytes)) declare_integer(_out, bytes, true);
        if (found_uint(bytes)) declare_integer(_out, bytes, false);
        for (unsigned char pt = 0; pt <= 80; ++pt)
        {
            if (found_fixed(bytes, pt)) declare_fixed(_out, bytes, pt, true);
            if (found_ufixed(bytes, pt)) declare_fixed(_out, bytes, pt, false);
        }
    }
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::declare_integer(
    ostream& _out, unsigned char _bytes, bool _signed
)
{
    EncodingData const DATA(_bytes, _signed);
    if (DATA.is_native_width && DATA.is_aligned_width) return;

    string const SYM = DATA.base + to_string(DATA.bits);
    if (DATA.is_native_width)
    {
        declare_padded_native(_out, SYM, DATA);
    }
    else
    {
        throw runtime_error("Primitives exceeding 64 bits not yet supported.");
    }
}

void PrimitiveTypeGenerator::declare_fixed(
    std::ostream& _out, unsigned char _bytes, unsigned char _pt, bool _signed
)
{
    EncodingData const DATA(_bytes, _signed);

    ostringstream sym_oss;
    if (!_signed) sym_oss << "u";
    sym_oss << "fixed" << DATA.bits << + "x" << to_string(_pt);
    string const SYM = sym_oss.str();

    if (DATA.is_native_width && DATA.is_aligned_width)
    {
        CStructDef decl(SYM, make_shared<CParams>(CParams{
            make_shared<CVarDecl>(DATA.base + to_string(DATA.bits) + "_t", "v")
        }));
        _out << decl << *decl.make_typedef(SYM + "_t");
    }
    else if (DATA.is_native_width)
    {
        declare_padded_native(_out, SYM, DATA);
    }
    else
    {
        throw runtime_error("Primitives exceeding 64 bits not yet supported.");
    }
}

void PrimitiveTypeGenerator::declare_padded_native(
    ostream& _out,
    string const& _sym,
    PrimitiveTypeGenerator::EncodingData const& _data
)
{
    // If the value is misaligned and native, it is 24, or over 32.
    unsigned short const PADDING = (_data.bits == 24 ? 32 : 64);
    CStructDef decl(_sym, make_shared<CParams>(CParams{
        make_shared<CVarDecl>(_data.base + to_string(PADDING) + "_t", "v")
    }));
    _out << decl << *decl.make_typedef(_sym + "_t");
}

// -------------------------------------------------------------------------- //

PrimitiveTypeGenerator::EncodingData::EncodingData(
    unsigned char _bytes, bool _signed
): bits(_bytes * 8)
 , is_native_width(bits <= 64)
 , is_aligned_width(is_power_of_two(_bytes))
 , base(_signed ? "int" : "uint")
{
}

// -------------------------------------------------------------------------- //

PrimitiveTypeGenerator::Visitor::Visitor(ASTNode const& _root)
{
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
            unsigned int const BIT_WIDTH = (int_type->numBits() / 8) - 1;
            if (int_type->isSigned())
            {
                m_uses_int[BIT_WIDTH] = true;
            }
            else
            {
                m_uses_uint[BIT_WIDTH] = true;
            }
        }
        else
        {
            throw runtime_error("Type category conflicts type class: Integer.");
        }
        break;
    case Type::Category::FixedPoint:
        if (auto fixed_type = dynamic_cast<FixedPointType const*>(_type))
        {
            unsigned int const BIT_WIDTH = (fixed_type->numBits() / 8) - 1;
            unsigned int const DCM_WIDTH = fixed_type->fractionalDigits();
            if (fixed_type->isSigned())
            {
                m_uses_fixed[BIT_WIDTH][DCM_WIDTH] = true;
            }
            else
            {
                m_uses_ufixed[BIT_WIDTH][DCM_WIDTH] = true;
            }
        }
        else
        {
            throw runtime_error("Type category conflicts type class: Fixed.");
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
