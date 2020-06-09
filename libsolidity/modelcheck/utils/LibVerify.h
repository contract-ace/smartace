/**
 * Provides an interface to generate libverify code.
 * 
 * @date 2020
 */

#pragma once

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
    // Appends a require statement to _block, conditioned on _cond.
    static void require(CBlockList & _block, CExprPtr _cond);

    // Produces a range between two values.
    static CExprPtr range(uint8_t _l, uint8_t _u, std::string const& _msg);

    // Generates a random byte of data, through a unique uninterpreted function.
    static CExprPtr byte(std::string const& _msg);

    // Appends a log statement to _block, with message _msg.
    static void log(CBlockList & _block, std::string _msg);

    // Appends a log statement to _block, with message _msg.
    static CExprPtr increase(CExprPtr _curr, bool _strict, std::string _msg);
};

// -------------------------------------------------------------------------- //

}
}
}
