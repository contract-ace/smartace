/**
 * Tests for libsolidity/modelcheck/utils/Primitives.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/utils/Primitives.h>

#include <boost/test/unit_test.hpp>

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(Utils_PrimitiveTests)

BOOST_AUTO_TEST_CASE(integers)
{
    BOOST_CHECK_EQUAL(PrimitiveToRaw::integer(32, true), "sol_raw_int32_t");
    BOOST_CHECK_EQUAL(PrimitiveToRaw::integer(40, true), "sol_raw_int40_t");
    BOOST_CHECK_EQUAL(PrimitiveToRaw::integer(32, false), "sol_raw_uint32_t");
    BOOST_CHECK_EQUAL(PrimitiveToRaw::integer(40, false), "sol_raw_uint40_t");
}

BOOST_AUTO_TEST_CASE(booleans)
{
    BOOST_CHECK_EQUAL(PrimitiveToRaw::boolean(), "sol_raw_uint8_t");
}

BOOST_AUTO_TEST_CASE(addresses)
{
    BOOST_CHECK_EQUAL(PrimitiveToRaw::address(), "sol_raw_uint160_t");
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
