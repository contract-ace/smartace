/**
 * Data and helper functions for generating the harness. This is meant to reduce
 * code duplication.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/utils/LibVerify.h>

#include <libsolidity/modelcheck/codegen/Literals.h>

#include <memory>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

IntegerType LibVerify::BYTE_TYPE(8, IntegerType::Modifier::Unsigned);

IntegerType LibVerify::INCR_TYPE(256, IntegerType::Modifier::Unsigned);

// -------------------------------------------------------------------------- //

CExprPtr LibVerify::make_require(CExprPtr _cond, string _msg)
{
    return make_property("sol_require", _cond, _msg);
}

CExprPtr LibVerify::make_assert(CExprPtr _cond, string _msg)
{
    return make_property("sol_assert", _cond, _msg);
}

void LibVerify::add_assert(CBlockList & _block, CExprPtr _cond, string _msg)
{
    add_property("sol_assert", _block, _cond, _msg);
}

void LibVerify::add_require(CBlockList & _block, CExprPtr _cond, string _msg)
{
    add_property("sol_require", _block, _cond, _msg);
}

CExprPtr LibVerify::range(
    size_t _loc, uint8_t _l, uint8_t _u, string const& _msg
)
{
    // Determines if there is more than one solution.
    auto lower = make_shared<CIntLiteral>(_l);
    if (_l + 1 == _u)
    {
        return lower;
    }
    else
    {
        CFuncCallBuilder builder("GET_ND_RANGE");
        builder.push(make_shared<CIntLiteral>(_loc));
        builder.push(move(lower));
        builder.push(make_shared<CIntLiteral>(_u));
        builder.push(make_shared<CStringLiteral>(_msg));
        return builder.merge_and_pop();
    }
}

CExprPtr LibVerify::byte(size_t _loc, string const& _msg)
{
    CFuncCallBuilder builder("GET_ND_BYTE");
    builder.push(make_shared<CIntLiteral>(_loc));
    builder.push(make_shared<CStringLiteral>(_msg));
    return builder.merge_and_pop();
}

void LibVerify::log(CBlockList & _block, string _msg)
{
    CArgList arglist{ make_shared<CStringLiteral>(_msg) };
    auto fn = make_shared<CFuncCall>("smartace_log", move(arglist));
    _block.push_back(fn->stmt());
}

CExprPtr LibVerify::increase(
    size_t _loc, CExprPtr _curr, bool _strict, string _msg
)
{
    CFuncCallBuilder builder("GET_ND_INCREASE");
    builder.push(make_shared<CIntLiteral>(_loc));
    builder.push(_curr);
    builder.push(_strict ? Literals::ONE : Literals::ZERO);
    builder.push(make_shared<CStringLiteral>(_msg));
    return builder.merge_and_pop();
}

void LibVerify::add_property(
    string _op, CBlockList & _block, CExprPtr _cond, string _msg
)
{
    _block.push_back(make_shared<CExprStmt>(make_property(_op, _cond, _msg)));
}

CExprPtr LibVerify::make_property(string _op, CExprPtr _cond, string _msg)
{
    CExprPtr msg_param;
    if (_msg.empty())
    {
        msg_param = Literals::ZERO;
    }
    else
    {
        msg_param = make_shared<CStringLiteral>(_msg);
    }
    return make_shared<CFuncCall>(_op, CArgList{_cond, msg_param});
}

// -------------------------------------------------------------------------- //

}
}
}
