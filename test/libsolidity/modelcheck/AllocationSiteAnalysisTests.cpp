/**
 * @date 2019
 * Tests for analyzing comtract references, along with contract allocations.
 */

#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <sstream>

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

BOOST_FIXTURE_TEST_SUITE(
    AllocationSiteAnalysis,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(valid_alloc_tests)
{
    char const* text = R"(
        contract X {}
        contract Y {}
        contract Z {}
        contract Q {}
        contract Test1 {
            int a;
            int b;
            constructor() public {
                a = 1;
                b = 2;
            }
            function f() public {
                a += 1;
                b += 2;
            }
        }
        contract Test2 {
            X x1;
            Y y1;
            Y y2;
            Z z1;
            Z z2;
            Z z3;
            constructor() public {
                x1 = new X();
                y1 = new Y();
                y2 = new Y();
                z1 = new Z();
                z2 = new Z();
                z3 = new Z();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* child_x = retrieveContractByName(unit, "X");
    auto const* child_y = retrieveContractByName(unit, "Y");
    auto const* child_z = retrieveContractByName(unit, "Z");

    auto const& test1 = *retrieveContractByName(unit, "Test1");
    auto const& test2 = *retrieveContractByName(unit, "Test2");

    NewCallSummary app1(test1);
    auto const& actviolations1 = app1.violations();
    auto const& actchild1 = app1.children();

    BOOST_CHECK(actviolations1.empty());
    BOOST_CHECK(actchild1.empty());

    NewCallSummary app2(test2);
    auto const& actviolations2 = app2.violations();
    auto const& actchild2 = app2.children();

    NewCallSummary::ChildType c1, c2, c3;
    c1.count = 1; c1.type = child_x;
    c2.count = 2; c2.type = child_y;
    c3.count = 3; c3.type = child_z;
    list<NewCallSummary::ChildType> expchild2({ c1, c2, c3 });

    BOOST_CHECK(actviolations2.empty());

    BOOST_CHECK_EQUAL(actchild2.size(), expchild2.size());
    for (auto a_itr = actchild2.begin(); a_itr != actchild2.end(); ++a_itr)
    {
        bool match = false;
        for (auto e_itr = expchild2.begin(); e_itr != expchild2.end(); ++e_itr)
        {
            bool type_match = (e_itr->type == a_itr->type);
            bool count_match = (e_itr->count == a_itr->count);
            match |= (type_match && count_match);
        }
        BOOST_CHECK(match);
    }
}

BOOST_AUTO_TEST_CASE(invalid_alloc_tests)
{
    char const* text = R"(
        contract X {}
        contract Y {}
        contract Z {}
        contract Q {}
        contract Test {
            X x;
            Y y;
            Z z;
            Q q;
            function f() public {
                x = new X();
            }
            function g() public {
                y = new Y();
            }
            function h() public {
                z = new Z();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* child_x = retrieveContractByName(unit, "X");
    auto const* child_y = retrieveContractByName(unit, "Y");
    auto const* child_z = retrieveContractByName(unit, "Z");

    auto const& test = *retrieveContractByName(unit, "Test");

    NewCallSummary app(test);
    BOOST_CHECK(app.children().empty());

    NewCallSummary::NewCall v1, v2, v3;
    v1.type = child_x; v1.context = test.definedFunctions()[0];
    v2.type = child_y; v2.context = test.definedFunctions()[1];
    v3.type = child_z; v3.context = test.definedFunctions()[2];
    list<NewCallSummary::NewCall> expect({v1, v2, v3});

    auto const& actual = app.violations();

    BOOST_CHECK_EQUAL(actual.size(), expect.size());
    for (auto a_itr = actual.begin(); a_itr != actual.end(); ++a_itr)
    {
        bool match = false;
        for (auto e_itr = expect.begin(); e_itr != expect.end(); ++e_itr)
        {
            bool type_match = (e_itr->type == a_itr->type);
            bool count_match = (e_itr->context == a_itr->context);
            match |= (type_match && count_match);
        }
        BOOST_CHECK(match);
    }
}

BOOST_AUTO_TEST_CASE(cost_analysis_test)
{
    char const* text = R"(
        contract X {}
        contract Y {
            constructor() public {
                new X();
                new X();
            }
        }
        contract Z {
            constructor() public {
                new X();
                new Y();
                new Y();
            }
        }
        contract Q {
            constructor() public {
                new X();
                new Y();
            }
        }
        contract R {
            constructor() public {
                new Q();
                new Z();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* child_x = retrieveContractByName(unit, "X");
    auto const* child_y = retrieveContractByName(unit, "Y");
    auto const* child_z = retrieveContractByName(unit, "Z");
    auto const* child_q = retrieveContractByName(unit, "Q");
    auto const* child_r = retrieveContractByName(unit, "R");

    NewCallGraph graph;
    graph.record(unit);
    graph.finalize();

    BOOST_CHECK_EQUAL(graph.cost_of(child_x), 1);
    BOOST_CHECK_EQUAL(graph.cost_of(child_y), 3);
    BOOST_CHECK_EQUAL(graph.cost_of(child_z), 8);
    BOOST_CHECK_EQUAL(graph.cost_of(child_q), 5);
    BOOST_CHECK_EQUAL(graph.cost_of(child_r), 14);
}

BOOST_AUTO_TEST_CASE(aggregate_exploit_test)
{
    char const* text = R"(
        contract X {}
        contract Y {
            function f() public {
                new X();
            }
        }
        contract Z {
            function g() public {
                new X();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* child_x = retrieveContractByName(unit, "X");
    auto const& child_y = *retrieveContractByName(unit, "Y");
    auto const& child_z = *retrieveContractByName(unit, "Z");

    NewCallGraph graph;
    graph.record(unit);
    graph.finalize();

    NewCallSummary::NewCall v1, v2;
    v1.type = child_x; v1.context = child_y.definedFunctions()[0];
    v2.type = child_x; v2.context = child_z.definedFunctions()[0];
    list<NewCallSummary::NewCall> expect({v1, v2});

    auto const& actual = graph.violations();

    BOOST_CHECK_EQUAL(actual.size(), expect.size());
    for (auto a_itr = actual.begin(); a_itr != actual.end(); ++a_itr)
    {
        bool match = false;
        for (auto e_itr = expect.begin(); e_itr != expect.end(); ++e_itr)
        {
            bool type_match = (e_itr->type == a_itr->type);
            bool count_match = (e_itr->context == a_itr->context);
            match |= (type_match && count_match);
        }
        BOOST_CHECK(match);
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
