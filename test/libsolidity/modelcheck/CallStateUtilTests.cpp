/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/utils/CallState.h
 */

#include <libsolidity/modelcheck/utils/CallState.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

namespace
{

static MagicType const BLOCK_TYPE(MagicType::Kind::Block);
static MagicType const MESSAGE_TYPE(MagicType::Kind::Message);

}

BOOST_FIXTURE_TEST_SUITE(
    CallStateUtilsTest,
    ::dev::solidity::test::AnalysisFramework
)

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
        CallStateUtilities::Field::Block
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

}
}
}
}
