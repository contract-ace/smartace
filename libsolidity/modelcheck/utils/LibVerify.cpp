/**
 * @date 2020
 * Data and helper functions for generating the harness. This is meant to reduce
 * code duplication.
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

CExprPtr LibVerify::range(uint8_t _l, uint8_t _u, string const& _msg)
{
    // Determines if there is more than one solution.
    auto lower = make_shared<CIntLiteral>(_l);
    if (_l + 1 == _u)
    {
        return lower;
    }
    else
    {
        auto msg = make_shared<CStringLiteral>(_msg);
        return make_shared<CFuncCall>(
            "nd_range", CArgList{ lower, make_shared<CIntLiteral>(_u), msg }
        );
    }
}

CExprPtr LibVerify::byte(string const& _msg)
{
    return make_shared<CFuncCall>(
        "nd_byte", CArgList{ make_shared<CStringLiteral>(_msg) }
    );
}

void LibVerify::log(CBlockList & _block, string _msg)
{
    auto fn = make_shared<CFuncCall>(
        "smartace_log", CArgList{ make_shared<CStringLiteral>(_msg)
    });
    _block.push_back(fn->stmt());
}

CExprPtr LibVerify::increase(CExprPtr _curr, bool _strict, string _msg)
{
    CFuncCallBuilder call("nd_increase");
    call.push(_curr);
    call.push(_strict ? Literals::ONE : Literals::ZERO);
    call.push(make_shared<CStringLiteral>(_msg));
    return call.merge_and_pop();
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
