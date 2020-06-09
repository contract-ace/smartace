/**
 * @date 2019
 * Test suite targeting function manipulation utilities.
 */

#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

using namespace std;
using namespace langutil;

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
    Inheritance,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(interface)
{
    char const* text = R"(
        contract A {
            function f() public pure {} // New
            function f(uint a) public pure {} // New
        }
        contract B is A {
            function f() public pure {}
            function f(uint a, uint b) public pure {} // New
            function g() public pure {} // New
        }
        contract C is B {
            function h() private pure {} // New
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");
    const auto& ctrt_c = *retrieveContractByName(unit, "C");

    BOOST_CHECK_EQUAL(FlatContract(ctrt_a).interface().size(), 2);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_b).interface().size(), 4);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_c).interface().size(), 4);
}

BOOST_AUTO_TEST_CASE(variables)
{
    char const* text = R"(
		contract A {
            int a; // New.
            int b; // New.
            int c; // New.
		}
        contract B is A {
            int a;
            int d; // New.
            int e; // New.
        }
        contract C is B {
            int b;
            int e;
            int f; // New.
        }
        contract D {
            int a;
            int g; // New.
            int h; // New.
        }
        contract E is C, D {
            int b;
            int g;
            int i; // New.
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");
    const auto& ctrt_c = *retrieveContractByName(unit, "C");
    const auto& ctrt_d = *retrieveContractByName(unit, "D");
    const auto& ctrt_e = *retrieveContractByName(unit, "E");

    BOOST_CHECK_EQUAL(FlatContract(ctrt_a).state_variables().size(), 3);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_b).state_variables().size(), 5);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_c).state_variables().size(), 6);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_d).state_variables().size(), 3);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_e).state_variables().size(), 9);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
