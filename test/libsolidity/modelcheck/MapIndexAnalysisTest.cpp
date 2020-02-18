/**
 * @date 2020
 * Tests for analyzing map index usage. Also tests that abstract domain
 * parameters are computed correctly.
 */

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>
#include <libsolidity/modelcheck/analysis/MapIndex.h>

using namespace std;
using langutil::SourceLocation;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

// -------------------------------------------------------------------------- //

BOOST_FIXTURE_TEST_SUITE(
    MapIndexAnalysis, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(address_to_int)
{
    char const* text = R"(
        contract X {
            int a;
            function f(address i) public {
                a += 1;
            }
            function g(address i) public pure {
                uint160(i);
            }
            function h(address i) public {
                a -= 1;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary;
    summary.record(ctrt);

    auto violations = summary.violations();
    BOOST_CHECK_EQUAL(violations.size(), 1);
    if (violations.size() == 1)
    {
        BOOST_CHECK_EQUAL(violations.front().context->name(), "g");
        BOOST_CHECK(
            violations.front().type == MapIndexSummary::ViolationType::Cast
        );
    }
}

BOOST_AUTO_TEST_CASE(int_to_address_valid)
{
    char const* text = R"(
        contract X {
            function f() public {
                address(1);
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary;
    summary.record(ctrt);
    BOOST_CHECK(summary.violations().empty());
}

BOOST_AUTO_TEST_CASE(int_to_address_invalid)
{
    char const* text = R"(
        contract X {
            struct A { uint160 i; }
            A a;
            function f(uint160 i) public {
                address(i);
            }
            function g(uint160 i) public {
                address(i + 1);
            }
            function h() public view {
                address(a.i);
            }
            function p() public pure returns (uint160) {
                return 10;
            }
            function q() public pure {
                address(p());
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary;
    summary.record(ctrt);

    auto violations = summary.violations();
    BOOST_CHECK_EQUAL(violations.size(), 4);
    for (auto itr : violations)
    {
        BOOST_CHECK(itr.type == MapIndexSummary::ViolationType::Mutate);
    }
}

BOOST_AUTO_TEST_CASE(comparisons)
{
    char const* text = R"(
        contract X {
            function f(address i, address j) public {
                i == j;
                i != j;
            }
            function g(address i, address j) public pure {
                i < j;
                i > j;
                i <= j;
                i >= j;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary;
    summary.record(ctrt);

    auto violations = summary.violations();
    BOOST_CHECK_EQUAL(violations.size(), 4);
    for (auto itr : violations)
    {
        BOOST_CHECK_EQUAL(itr.context->name(), "g");
        BOOST_CHECK(itr.type == MapIndexSummary::ViolationType::Compare);
    }
}

BOOST_AUTO_TEST_CASE(literals)
{
    char const* text = R"(
        contract X {
            function f() public pure {
                address(4);
                address(10);
                address i;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary;
    summary.record(ctrt);

    auto literals = summary.literals();
    BOOST_CHECK_EQUAL(literals.size(), 3);
    if (literals.size() == 3)
    {
        BOOST_CHECK(literals.find(dev::u256(0)) != literals.end());
        BOOST_CHECK(literals.find(dev::u256(4)) != literals.end());
        BOOST_CHECK(literals.find(dev::u256(10)) != literals.end());
    }
}

BOOST_AUTO_TEST_CASE(mixed_summary)
{
    char const* text = R"(
        contract X {
            function f() public pure {
                address i;
            }
            function g(uint160 i) public pure {
                address(i);
            }
        }
        contract Y {
            function f() public pure {
                address i = address(56);
            }
            function g(uint160 i) public pure {
                address(i);
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt1 = *retrieveContractByName(unit, "X");
    auto const& ctrt2 = *retrieveContractByName(unit, "Y");

    MapIndexSummary summary;

    summary.record(ctrt1);
    BOOST_CHECK_EQUAL(summary.violations().size(), 1);
    BOOST_CHECK_EQUAL(summary.literals().size(), 1);

    summary.record(ctrt2);
    BOOST_CHECK_EQUAL(summary.violations().size(), 2);
    BOOST_CHECK_EQUAL(summary.literals().size(), 2);
}

BOOST_AUTO_TEST_CASE(interference_count)
{
    char const* text = R"(
        contract X {
            function f() public pure { }
            function f(address i, address j) public pure { }
            function f(address j) public pure { }
        }
        contract Y {
            address k;
            address l;
            function f() public pure { }
            function f(address i, address j) public pure { }
            function f(address j) public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt1 = *retrieveContractByName(unit, "X");
    // auto const& ctrt2 = *retrieveContractByName(unit, "Y");

    MapIndexSummary summary;

    summary.record(ctrt1);
    BOOST_CHECK_EQUAL(summary.max_interference(), 3);

    // TODO(scottwe): enable with variable addresses.
    // summary.record(ctrt2);
    // BOOST_CHECK_EQUAL(summary.max_interference(), 5);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
