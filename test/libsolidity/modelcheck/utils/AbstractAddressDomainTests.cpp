/**
 * Specific tests for libsolidity/modelcheck/utils/AbstractAddressDomain.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/utils/AbstractAddressDomain.h>

#include <boost/test/unit_test.hpp>

#include <set>

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

BOOST_AUTO_TEST_SUITE(Utils_AbstractAddressDomainTests)

BOOST_AUTO_TEST_CASE(unique_literals)
{
    set<string> l;
    for (size_t i = 0; i < UINT16_MAX; ++i)
    {
        BOOST_CHECK(l.insert(AbstractAddressDomain::literal_name(i)).second);
    }
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
