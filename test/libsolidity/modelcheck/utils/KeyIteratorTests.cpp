/**
 * Tests for libsolidity/modelcheck/utils/KeyIterator.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/utils/KeyIterator.h>

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

BOOST_AUTO_TEST_SUITE(Utils_KeyIteratorTess)

BOOST_AUTO_TEST_CASE(zero_cases)
{
    KeyIterator itr_0(0, 0);
    KeyIterator itr_1(10, 0);
    KeyIterator itr_2(0, 10);

    BOOST_CHECK(!itr_0.is_full());
    BOOST_CHECK(!itr_1.is_full());
    BOOST_CHECK(!itr_2.is_full());

    BOOST_CHECK(!itr_0.next());
    BOOST_CHECK(!itr_1.next());
    BOOST_CHECK(!itr_2.next());

    BOOST_CHECK(itr_0.size() == 0);
    BOOST_CHECK(itr_1.size() == 0);
    BOOST_CHECK(itr_2.size() == 0);
}

BOOST_AUTO_TEST_CASE(dim_3X2)
{
    KeyIterator itr(3, 2);

    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK_EQUAL(itr.size(), 0);
    BOOST_CHECK(itr.next()); // So far: 0
    BOOST_CHECK_EQUAL(itr.size(), 1);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK(itr.next()); // So far: 0, 0
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_0_0");
    BOOST_CHECK(itr.next()); // So far: 0, 1
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_0_1");
    BOOST_CHECK(itr.next()); // So far: 0, 2
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_0_2");
    BOOST_CHECK(itr.next()); // So far: 1
    BOOST_CHECK_EQUAL(itr.size(), 1);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK(itr.next()); // So far: 1, 0
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_1_0");
    BOOST_CHECK(itr.next()); // So far: 1, 1
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_1_1");
    BOOST_CHECK(itr.next()); // So far: 1, 2
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_1_2");
    BOOST_CHECK(itr.next()); // So far: 2
    BOOST_CHECK_EQUAL(itr.size(), 1);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK(itr.next()); // So far: 2, 0
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_2_0");
    BOOST_CHECK(itr.next()); // So far: 2, 1
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_2_1");
    BOOST_CHECK(itr.next()); // So far: 2, 2
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_2_2");
    BOOST_CHECK(!itr.next());
}

BOOST_AUTO_TEST_CASE(dim_3X2_offset_2)
{
    KeyIterator itr(3, 2, 2);

    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK_EQUAL(itr.size(), 0);
    BOOST_CHECK(itr.next()); // So far: 0
    BOOST_CHECK_EQUAL(itr.size(), 1);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK(itr.next()); // So far: 0, 0
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_0_0");
    BOOST_CHECK(itr.next()); // So far: 0, 1
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_0_1");
    BOOST_CHECK(itr.next()); // So far: 0, 2
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_0_2");
    BOOST_CHECK(itr.next()); // So far: 1
    BOOST_CHECK_EQUAL(itr.size(), 1);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK(itr.next()); // So far: 1, 0
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_1_0");
    BOOST_CHECK(itr.next()); // So far: 1, 1
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_1_1");
    BOOST_CHECK(itr.next()); // So far: 1, 2
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_1_2");
    BOOST_CHECK(itr.next()); // So far: 2
    BOOST_CHECK_EQUAL(itr.size(), 1);
    BOOST_CHECK(!itr.is_full());
    BOOST_CHECK(itr.next()); // So far: 2, 0
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_2_0");
    BOOST_CHECK(itr.next()); // So far: 2, 1
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_2_1");
    BOOST_CHECK(itr.next()); // So far: 2, 2
    BOOST_CHECK_EQUAL(itr.size(), 2);
    BOOST_CHECK(itr.is_full());
    BOOST_CHECK_EQUAL(itr.suffix(), "_2_2");
    BOOST_CHECK(!itr.next());
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}

