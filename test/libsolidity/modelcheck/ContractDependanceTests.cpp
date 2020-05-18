/**
 * @date 2020
 * Specific tests for libsolidity/modelcheck/analysis/ContractDependance.cpp
 */

#include <libsolidity/modelcheck/analysis/ContractDependance.h>

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

BOOST_FIXTURE_TEST_SUITE(
    ContractDependanceTests,
    ::dev::solidity::test::AnalysisFramework
)

// Tests that unused contracts are hidden.
BOOST_AUTO_TEST_CASE(prunes_contracts)
{
    char const* text = R"(
        contract X {}
        contract XX is X {}
        contract Y {}
        contract Z {}
        contract WithX {
            X x;
            constructor() public { x = new XX(); }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(ast, "X");
    auto const* ctrt_xx = retrieveContractByName(ast, "XX");
    auto const* ctrt_y = retrieveContractByName(ast, "Y");
    auto const* ctrt_z = retrieveContractByName(ast, "Z");
    auto const* ctrt_wx = retrieveContractByName(ast, "WithX");

    NewCallGraph graph;
    graph.record(ast);
    graph.finalize();

    ContractDependance deps(
        ModelDrivenContractDependance({ ctrt_wx, ctrt_y }, graph)
    );

    BOOST_CHECK(!deps.is_deployed(ctrt_x));
    BOOST_CHECK(deps.is_deployed(ctrt_xx));
    BOOST_CHECK(deps.is_deployed(ctrt_y));
    BOOST_CHECK(!deps.is_deployed(ctrt_z));
    BOOST_CHECK(deps.is_deployed(ctrt_wx));
}

// Tests that the correct interfaces are extracted per contract.
BOOST_AUTO_TEST_CASE(interfaces)
{
    char const* text = R"(
        interface IX {
            function f() external view;
        }
        contract X is IX {
            function f() public view {}
            function g() public view {}
        }
        contract Y is X {
            function f() public view {}
            function h() public view {}
        }
        contract Z is Y {
            function q() public view {}
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const* ctrt = retrieveContractByName(ast, "Z");

    NewCallGraph graph;
    graph.record(ast);
    graph.finalize();

    ContractDependance deps(ModelDrivenContractDependance({ ctrt }, graph));

    BOOST_CHECK_EQUAL(deps.get_interface(ctrt).size(), 4);
}

// Tests that unused contracts are hidden.
BOOST_AUTO_TEST_CASE(supercalls)
{
    char const* text = R"(
        contract X {
            int x;
            function f() public view { assert(x == x); }
        }
        contract Y is X {
            function f() public view { assert(x == x); }
        }
        contract Z is Y {
            function f() public view {
                assert(x == x);
                super.f();
            }
        }
        contract Q is Z { }
        contract R is Q {
            function f() public view {
                assert(x == x);
                super.f();
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_r = retrieveContractByName(unit, "R");
    auto const* ctrt_z = retrieveContractByName(unit, "Z");
    auto const* ctrt_y = retrieveContractByName(unit, "Y");

    NewCallGraph graph;
    graph.record(unit);
    graph.finalize();

    ContractDependance deps(ModelDrivenContractDependance({ ctrt_r }, graph));

    auto const& supers = deps.get_superchain(ctrt_r->definedFunctions()[0]);
    BOOST_CHECK_EQUAL(ctrt_r->definedFunctions()[0]->name(), "f");
    BOOST_CHECK_EQUAL(ctrt_z->definedFunctions()[0]->name(), "f");
    BOOST_CHECK_EQUAL(ctrt_y->definedFunctions()[0]->name(), "f");
    BOOST_CHECK_EQUAL(supers.size(), 3);

    BOOST_CHECK(supers[0] == ctrt_r->definedFunctions()[0]);
    BOOST_CHECK(supers[1] == ctrt_z->definedFunctions()[0]);
    BOOST_CHECK(supers[2] == ctrt_y->definedFunctions()[0]);
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
