/**
 * Provides an interface to generate libverify code.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/Types.h>
#include <libsolidity/modelcheck/codegen/Details.h>

#include <cstdint>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Generates interface code to libverify.
 */
class LibVerify
{
public:
    // The type of ND_GET_BYTE and ND_GET_RANGE.
    static IntegerType BYTE_TYPE;

    // The type of ND_GET_INCREASE.
    static IntegerType INCR_TYPE;

    // Generates a call to `sol_require(<_cond>, <_msg>)`.
    static CExprPtr make_require(CExprPtr _cond, std::string _msg = "");

    // Generates a call to `sol_assert(<_cond>, <_msg>)`.
    static CExprPtr make_assert(CExprPtr _cond, std::string _msg = "");

    // Appends to _block a call to `sol_require(<_cond>, <_msg>)`.
    static void
        add_assert(CBlockList & _block, CExprPtr _cond, std::string _msg = "");

    // Appends to _block a call to `sol_assert(<_cond>, <_msg>)`.
    static void
        add_require(CBlockList & _block, CExprPtr _cond, std::string _msg = "");

    // Produces a range between two values.
    static CExprPtr range(
        size_t _loc, uint8_t _l, uint8_t _u, std::string const& _msg
    );

    // Generates a random byte of data, through a unique uninterpreted function.
    static CExprPtr byte(size_t _loc, std::string const& _msg);

    // Appends a log statement to _block, with message _msg.
    static void log(CBlockList & _block, std::string _msg);

    // Appends a log statement to _block, with message _msg.
    static CExprPtr increase(
        size_t _loc, CExprPtr _curr, bool _strict, std::string _msg
    );

private:
    // Appends to _block a call to `<_op>(<_cond>, <_msg>)`.
    static void add_property(
        std::string _op, CBlockList & _block, CExprPtr _cond, std::string _msg
    );

    // Returns a call to `<_op>(<_cond>, <_msg>)`.
    static CExprPtr
        make_property(std::string _op, CExprPtr _cond, std::string _msg);
};

// -------------------------------------------------------------------------- //

}
}
}
