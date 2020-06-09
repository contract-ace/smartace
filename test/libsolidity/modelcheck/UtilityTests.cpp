/**
 * @date 2019
 * Basic tests to detect faulty utilities. As most utilities are generated
 * through template metaprogramming, the test suite is likely incomplete.
 * 
 * Targets libsolidity/modelcheck/Utility.h.
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

BOOST_AUTO_TEST_SUITE(Utilities)

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

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}

