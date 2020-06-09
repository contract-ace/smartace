/**
 * @date 2019
 * Test suite targeting AST manipulation utilities.
 */

#include <libsolidity/modelcheck/utils/Function.h>

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
    FunctionUtils,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(function_collision)
{
    char const* text = R"(
		contract A {
			function f(int a, uint b) public pure {}
			function f(int a) public pure {}
			function f() public pure {}
			function g() public pure {}
		}
        contract B {
			function f(int c, uint d) public pure {}
			function f(int c) public pure {}
			function f() public pure {}
			function g() public pure {}
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");

    for (unsigned int i = 0; i < 4; ++i)
    {
        auto func_a_i = ctrt_a.definedFunctions()[i];
        auto func_b_i = ctrt_b.definedFunctions()[i];
        BOOST_CHECK(collid(*func_a_i, *func_a_i));
        BOOST_CHECK(collid(*func_a_i, *func_b_i));
        BOOST_CHECK(collid(*func_b_i, *func_b_i));
        for (unsigned int j = 0; j < 4; ++j)
        {
            auto func_a_j = ctrt_a.definedFunctions()[j];
            auto func_b_j = ctrt_b.definedFunctions()[j];
            if (i != j) BOOST_CHECK(!collid(*func_a_i, *func_b_j));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
