#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Primitives.h>
#include <libsolidity/modelcheck/utils/Types.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

NondetSourceRegistry::NondetSourceRegistry(
    shared_ptr<AnalysisStack const> _stack
): m_stack(_stack) {}

CExprPtr NondetSourceRegistry::byte(string _msg)
{
    size_t loc = m_registry.size();
    m_registry.push_back(&LibVerify::BYTE_TYPE);

    return LibVerify::byte(loc, _msg);
}

CExprPtr NondetSourceRegistry::range(uint8_t _l, uint8_t _u, string const& _msg)
{
    size_t loc = m_registry.size();
    m_registry.push_back(&LibVerify::BYTE_TYPE);

    return LibVerify::range(loc, _l, _u, _msg);
}

CExprPtr NondetSourceRegistry::increase(
    CExprPtr _curr, bool _strict, string _msg
)
{
    size_t loc = m_registry.size();
    m_registry.push_back(&LibVerify::INCR_TYPE);

    return LibVerify::increase(loc, _curr, _strict, _msg);
}

CExprPtr NondetSourceRegistry::raw_val(Type const& _type, string const& _msg)
{
    if (!is_simple_type(_type))
    {
        throw std::runtime_error("raw_val expects a simple type.");
    }

    auto const CATEGORY = unwrap(_type).category();
    if (CATEGORY == Type::Category::Bool)
    {
        return range(0, 2, _msg);
    }
    else if (CATEGORY == Type::Category::Address)
    {
        size_t addr_count = m_stack->addresses()->size();
        return range(0, addr_count, _msg);
    }
    else
    {
        size_t loc = m_registry.size();
        m_registry.push_back(&_type);

        string macroname = "GET_ND_UINT";
        if (simple_is_signed(_type))
        {
            macroname = "GET_ND_INT";
        }

        CFuncCallBuilder call(macroname);
        call.push(make_shared<CIntLiteral>(loc));
        call.push(make_shared<CIntLiteral>(simple_bit_count(_type)));
        call.push(make_shared<CStringLiteral>(_msg));
        return call.merge_and_pop();
    }
}

CExprPtr NondetSourceRegistry::simple_val(Type const& _type, string const& _msg)
{
    auto nd_val = raw_val(_type, _msg);
    return InitFunction::wrap(_type, move(nd_val));
}

CExprPtr NondetSourceRegistry::val(TypeName const& _type, string const& _msg)
{
    if (has_simple_type(_type))
    {
        return simple_val(*_type.annotation().type, _msg);
    }
    else
    {
        string name = m_stack->types()->get_name(_type);
        return make_shared<CFuncCall>("ND_" + name, CArgList{});
    }
}

CExprPtr NondetSourceRegistry::val(Declaration const& _decl, string const& _msg)
{
    if (has_simple_type(_decl))
    {
        return simple_val(*_decl.type(), _msg);
    }
    else
    {
        string name = m_stack->types()->get_name(_decl);
        return make_shared<CFuncCall>("ND_" + name, CArgList{});
    }
}

void NondetSourceRegistry::print(std::ostream& _stream)
{
    CParams const args;
    CFuncDef::Modifier const mod = CFuncDef::Modifier::EXTERN;
    for (size_t i = 0; i < m_registry.size(); ++i)
    {
        auto const& SRC = (*m_registry[i]);

        auto const BITS = simple_bit_count(SRC);
        auto const SIGN = simple_is_signed(SRC);
        auto const TYPE = PrimitiveToRaw::integer(BITS, SIGN);
        auto const NAME = "sea_nd_" + to_string(i);

        auto id = make_shared<CVarDecl>(TYPE, NAME);
        _stream << CFuncDef(move(id), args, nullptr, mod);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
