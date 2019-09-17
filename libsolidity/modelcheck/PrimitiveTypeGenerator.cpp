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

PrimitiveTypeGenerator::PrimitiveTypeGenerator()
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
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::record(ASTNode const& _root)
{
    _root.accept(*this);
}

// -------------------------------------------------------------------------- //

bool PrimitiveTypeGenerator::found_bool() const { return m_uses_bool; }

bool PrimitiveTypeGenerator::found_address() const { return m_uses_address; }

bool PrimitiveTypeGenerator::found_int(uint8_t _bytes) const
{
    return m_uses_int[_bytes - 1];
}

bool PrimitiveTypeGenerator::found_uint(uint8_t _bytes) const
{
    return m_uses_uint[_bytes - 1];
}

bool PrimitiveTypeGenerator::found_fixed(uint8_t _bytes, uint8_t _d) const
{
    return m_uses_fixed[_bytes - 1][_d];
}

bool PrimitiveTypeGenerator::found_ufixed(uint8_t _bytes, uint8_t _d) const
{
    return m_uses_ufixed[_bytes - 1][_d];
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::print(ostream& _out) const
{
    if (found_bool())
    {
        string const RAW_DATA = "sol_raw_uint8_t";
        string const RAW_STRUCT = "sol_bool";
        declare_primitive(_out, RAW_STRUCT, RAW_DATA);
    }
    if (found_address())
    {
        string const RAW_DATA = "sol_raw_uint160_t";
        string const RAW_STRUCT = "sol_address";
        declare_primitive(_out, RAW_STRUCT, RAW_DATA);

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
ostream& _out, uint8_t _bytes, bool _signed
)
{
    ostringstream sym_oss;
    sym_oss << "int" << (_bytes * 8);
    string const SYM = sym_oss.str();
 
    declare_numeric(_out, SYM, _bytes, _signed);
}

void PrimitiveTypeGenerator::declare_fixed(
    std::ostream& _out, uint8_t _bytes, uint8_t _pt, bool _signed
)
{
    ostringstream sym_oss;
    sym_oss << "fixed" << (_bytes * 8) << "x" << static_cast<uint16_t>(_pt);
    string const SYM = sym_oss.str();

    declare_numeric(_out, SYM, _bytes, _signed);
}

void PrimitiveTypeGenerator::declare_numeric(
    ostream& _out, string const& _sym, uint8_t _bytes, bool _signed
)
{
    string const SIGN = (_signed ? "" : "u");
    string const WRAPPER = "sol_" + SIGN + _sym;

    ostringstream data_oss;
    data_oss << "sol_raw_" << SIGN << "int" << (_bytes * 8) << "_t";
    string const DATA = data_oss.str();

    declare_primitive(_out, WRAPPER, DATA);
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::declare_primitive(
    ostream& _out, string const& _type, string const& _data
)
{
    CStructDef decl(_type, make_shared<CParams>(CParams{
        make_shared<CVarDecl>(_data, "v")
    }));

    string const TYPEDEF = _type + "_t";

    auto id = make_shared<CVarDecl>(TYPEDEF, "Init_" + TYPEDEF);
    auto input_dcl = make_shared<CVarDecl>(_data, "v");
    auto tmp_dcl = make_shared<CVarDecl>(TYPEDEF, "tmp");

    auto block = make_shared<CBlock>(CBlockList{
        tmp_dcl,
        tmp_dcl->access("v")->assign(input_dcl->id())->stmt(),
        make_shared<CReturn>(tmp_dcl->id())
    });

    _out << decl << *decl.make_typedef(TYPEDEF)
         << CFuncDef(id, {input_dcl}, block, CFuncDef::Modifier::INLINE);
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::endVisit(UsingForDirective const& _node)
{
    process_type(_node.typeName()->annotation().type);
}

void PrimitiveTypeGenerator::endVisit(VariableDeclaration const& _node)
{
    process_type(_node.type());
}

void PrimitiveTypeGenerator::endVisit(ElementaryTypeName const& _node)
{
    process_type(_node.annotation().type);
}

void PrimitiveTypeGenerator::endVisit(ElementaryTypeNameExpression const& _node)
{
    process_type(_node.annotation().type);
}

void PrimitiveTypeGenerator::endVisit(FunctionCall const& _node)
{
    if (_node.annotation().kind == FunctionCallKind::FunctionCall)
    {
        auto const* functype = dynamic_cast<FunctionType const*>(
            _node.expression().annotation().type
        );

        if (functype)
        {
            switch (functype->kind())
            {
            case FunctionType::Kind::Assert:
            case FunctionType::Kind::Require:
                m_uses_bool = true;
                break;
	        case FunctionType::Kind::Send:
	        case FunctionType::Kind::Transfer:
                m_uses_address = true;
                m_uses_uint[31] = true;
                break;
            default: break;
            }
        }
    }
}

// -------------------------------------------------------------------------- //

void PrimitiveTypeGenerator::process_type(Type const* _type)
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
