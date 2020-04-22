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

    MapIndexSummary summary(false, 5, 5);
    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);

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

    MapIndexSummary summary(false, 5, 5);
    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);
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

    MapIndexSummary summary(false, 5, 5);
    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);

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

    MapIndexSummary summary(false, 5, 5);
    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);

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

    MapIndexSummary summary(false, 5, 5);
    BOOST_CHECK_EQUAL(summary.size(), 11);

    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);

    auto literals = summary.literals();
    BOOST_CHECK_EQUAL(literals.size(), 3);
    BOOST_CHECK_EQUAL(summary.size(), 14);
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

    MapIndexSummary summary(false, 5, 5);

    summary.extract_literals(ctrt1);
    summary.compute_interference(ctrt1);
    BOOST_CHECK_EQUAL(summary.violations().size(), 1);
    BOOST_CHECK_EQUAL(summary.literals().size(), 1);

    summary.extract_literals(ctrt2);
    summary.compute_interference(ctrt2);
    BOOST_CHECK_EQUAL(summary.violations().size(), 2);
    BOOST_CHECK_EQUAL(summary.literals().size(), 2);
}

BOOST_AUTO_TEST_CASE(basic_interference_count)
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
    auto const& ctrt2 = *retrieveContractByName(unit, "Y");

    MapIndexSummary summary1(false, 5, 5);
    summary1.extract_literals(ctrt1);
    summary1.compute_interference(ctrt1);
    BOOST_CHECK_EQUAL(summary1.max_interference(), 3);

    MapIndexSummary summary2(false, 5, 5);
    summary2.extract_literals(ctrt1);
    summary2.extract_literals(ctrt2);
    summary2.compute_interference(ctrt1);
    summary2.compute_interference(ctrt2);
    BOOST_CHECK_EQUAL(summary2.max_interference(), 5);
}

BOOST_AUTO_TEST_CASE(compound_interference_count)
{
    char const* text = R"(
        contract X {
            struct A {
                address x;
                address y;
            }
            struct B {
                A a1;
                A a2;
                address z;
            }
            B b;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary(false, 5, 5);

    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);
    BOOST_CHECK_EQUAL(summary.max_interference(), 6);
}

BOOST_AUTO_TEST_CASE(basic_map_of_addrs_count)
{
    char const* text = R"(
        contract X {
            mapping(address => mapping(address => address)) m;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary(false, 2, 0);

    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);
    BOOST_CHECK_EQUAL(summary.max_interference(), 10);
}

BOOST_AUTO_TEST_CASE(basic_map_of_structs_count)
{
    char const* text = R"(
        contract X {
            struct A {
                address x;
                address y;
            }
            mapping(address => mapping(address => A)) m;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary(false, 2, 0);

    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);
    BOOST_CHECK_EQUAL(summary.max_interference(), 19);
}

BOOST_AUTO_TEST_CASE(inherited_addr_count)
{
    char const* text = R"(
        contract Y {
            address y;
        }
        contract X is Y {
            address x;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary(false, 5, 5);

    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);
    BOOST_CHECK_EQUAL(summary.max_interference(), 3);
}

BOOST_AUTO_TEST_CASE(concrete_map_test)
{
    char const* text = R"(
        contract X {
            mapping(address => mapping(address => address)) m;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    MapIndexSummary summary(true, 2, 0);

    summary.extract_literals(ctrt);
    summary.compute_interference(ctrt);
    BOOST_CHECK_EQUAL(summary.max_interference(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
