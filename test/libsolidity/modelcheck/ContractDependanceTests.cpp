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

BOOST_AUTO_TEST_CASE(roi_for_calls)
{
    char const* text = R"(
        contract X {
            function f() public view {}
            function g() public view {}
            function h() public view {}
        }
        contract Y {
            X x;
            constructor() public { x = new X(); }
            function f() public view { x.f(); }
            function g() public view {}
            function h() public view { x.h(); }
        }
        contract Z {
            Y y;
            constructor() public { y = new Y(); }
            function f() public view { y.f(); y.h(); f(); }
            function g() public view { y.g(); }
            function h() public view {}
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt = retrieveContractByName(unit, "Z");

    NewCallGraph graph;
    graph.record(unit);
    graph.finalize();

    ContractDependance deps(ModelDrivenContractDependance({ ctrt }, graph));

    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[1]->name(), "f");
    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[2]->name(), "g");
    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[3]->name(), "h");

    BOOST_CHECK_EQUAL(
        deps.get_function_roi(ctrt->definedFunctions()[1]).size(), 5
    );
    BOOST_CHECK_EQUAL(
        deps.get_function_roi(ctrt->definedFunctions()[2]).size(), 2
    );
    BOOST_CHECK_EQUAL(
        deps.get_function_roi(ctrt->definedFunctions()[3]).size(), 1
    );
}

BOOST_AUTO_TEST_CASE(roi_for_map)
{
    char const* text = R"(
        contract X {
            mapping(address => int) m1;
            mapping(address => mapping(address => int)) m2;
            function f() public view { m1[address(1)]; }
            function g() public view { m1[address(1)]; }
            function h() public view { m2[address(1)][address(1)]; }
        }
        contract Y {
            X x;
            mapping(address => int) m1;
            constructor() public { x = new X(); }
            function calls_1a() public view { m1[address(1)]; }
            function calls_1b() public view { x.f(); }
            function calls_1c() public view { x.f(); x.g(); }
            function calls_2() public view { x.f(); x.h(); }
            function calls_3() public view { calls_1a(); x.f(); x.h(); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt = retrieveContractByName(unit, "Y");

    NewCallGraph graph;
    graph.record(unit);
    graph.finalize();

    ContractDependance deps(ModelDrivenContractDependance({ ctrt }, graph));

    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[1]->name(), "calls_1a");
    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[2]->name(), "calls_1b");
    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[3]->name(), "calls_1c");
    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[4]->name(), "calls_2");
    BOOST_CHECK_EQUAL(ctrt->definedFunctions()[5]->name(), "calls_3");

    BOOST_CHECK_EQUAL(deps.get_map_roi(ctrt->definedFunctions()[1]).size(), 1);
    BOOST_CHECK_EQUAL(deps.get_map_roi(ctrt->definedFunctions()[2]).size(), 1);
    BOOST_CHECK_EQUAL(deps.get_map_roi(ctrt->definedFunctions()[3]).size(), 1);
    BOOST_CHECK_EQUAL(deps.get_map_roi(ctrt->definedFunctions()[4]).size(), 2);
    BOOST_CHECK_EQUAL(deps.get_map_roi(ctrt->definedFunctions()[5]).size(), 3);
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
