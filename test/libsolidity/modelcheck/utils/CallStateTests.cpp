/**
 * Tests for libsolidity/modelcheck/utils/CallState.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/utils/CallState.h>

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

namespace
{

static MagicType const BLOCK_TYPE(MagicType::Kind::Block);
static MagicType const MESSAGE_TYPE(MagicType::Kind::Message);

}

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(Utils_CallStateTests)

BOOST_AUTO_TEST_CASE(handling_magic_types)
{
    BOOST_CHECK(
        CallStateUtilities::parse_magic_type(BLOCK_TYPE, "number")
        ==
        CallStateUtilities::Field::Block
    );
    BOOST_CHECK(
        CallStateUtilities::parse_magic_type(BLOCK_TYPE, "timestamp")
        ==
        CallStateUtilities::Field::Timestamp
    );
    BOOST_CHECK(
        CallStateUtilities::parse_magic_type(MESSAGE_TYPE, "sender")
        ==
        CallStateUtilities::Field::Sender
    );
    BOOST_CHECK(
        CallStateUtilities::parse_magic_type(MESSAGE_TYPE, "value")
        ==
        CallStateUtilities::Field::Value
    );
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
