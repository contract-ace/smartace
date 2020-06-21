/**
 * Tests for libsolidity/modelcheck/utils/General.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/utils/General.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(Utils_GeneralTess)

BOOST_AUTO_TEST_CASE(scope_swap)
{
    int my_var = 0;

    ScopedSwap<int> scope_1(my_var, 1);
    BOOST_CHECK_EQUAL(my_var, 1);
    BOOST_CHECK_EQUAL(scope_1.old(), 0);
    {
        ScopedSwap<int> scope_2(my_var, 2);
        BOOST_CHECK_EQUAL(my_var, 2);
        BOOST_CHECK_EQUAL(scope_2.old(), 1);
        {
            ScopedSwap<int> scope_2(my_var, 3);
            BOOST_CHECK_EQUAL(my_var, 3);
            BOOST_CHECK_EQUAL(scope_2.old(), 2);
        }
        BOOST_CHECK_EQUAL(my_var, 2);
    }
    BOOST_CHECK_EQUAL(my_var, 1);
}

BOOST_AUTO_TEST_CASE(pow_special_cases)
{
    BOOST_CHECK_EQUAL(fast_pow(0, 0), 0);
    BOOST_CHECK_EQUAL(fast_pow(0, 10), 0);
    BOOST_CHECK_EQUAL(fast_pow(1, 0), 1);
    BOOST_CHECK_EQUAL(fast_pow(1, 10), 1);
    BOOST_CHECK_EQUAL(fast_pow(100, 0), 1);
}

BOOST_AUTO_TEST_CASE(pow_general)
{
    uint64_t base_2 = 2;
    uint64_t base_3 = 3;
    for (size_t i = 1; i <= 5; ++i)
    {
        BOOST_CHECK_EQUAL(fast_pow(2, i), base_2);
        BOOST_CHECK_EQUAL(fast_pow(3, i), base_3);
        base_2 *= 2;
        base_3 *= 3;
    }
}


BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}

