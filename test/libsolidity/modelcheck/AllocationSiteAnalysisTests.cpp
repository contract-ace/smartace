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
        contract Test3 {
            X x;
            constructor() public {
                x = new X();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* child_x = retrieveContractByName(unit, "X");
    auto const* child_y = retrieveContractByName(unit, "Y");
    auto const* child_z = retrieveContractByName(unit, "Z");

    auto const& test1 = *retrieveContractByName(unit, "Test1");
    auto const& test2 = *retrieveContractByName(unit, "Test2");
    auto const& test3 = *retrieveContractByName(unit, "Test3");

    NewCallSummary app1(test1);
    auto const& actviolations1 = app1.violations();
    auto const& actchild1 = app1.children();

    BOOST_CHECK(actviolations1.empty());
    BOOST_CHECK(actchild1.empty());

    NewCallSummary app2(test2);
    auto const& actviolations2 = app2.violations();
    auto const& actchild2 = app2.children();

    NewCallSummary::NewCall c1, c2, c3, c4, c5, c6;
    c1.type = child_x;
    c2.type = child_y;
    c3.type = child_y;
    c4.type = child_z;
    c5.type = child_z;
    c6.type = child_z;
    NewCallSummary::CallGroup expchild2({ c1, c2, c3, c4, c5, c6 });

    BOOST_CHECK(actviolations2.empty());

    BOOST_CHECK_EQUAL(actchild2.size(), expchild2.size());
    for (auto a_itr = actchild2.begin(); a_itr != actchild2.end(); ++a_itr)
    {
        bool match = false;
        for (auto e_itr = expchild2.begin(); e_itr != expchild2.end(); ++e_itr)
        {
            match |= (e_itr->type == a_itr->type);
        }
        BOOST_CHECK(match);
    }

    NewCallSummary app3(test3);
    BOOST_CHECK_EQUAL(app3.children().size(), 1);
    if (app3.children().size() == 1)
    {
        BOOST_CHECK_EQUAL(app3.children().front().dest, test3.stateVariables()[0]);
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
            constructor() public {
                Q myQ = new Q();
            }
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
    auto const* child_q = retrieveContractByName(unit, "Q");

    auto const& test = *retrieveContractByName(unit, "Test");

    NewCallSummary app(test);
    BOOST_CHECK(app.children().empty());

    NewCallSummary::NewCall v1, v2, v3, v4;
    v1.type = child_q; v1.context = test.definedFunctions()[0];
    v2.type = child_x; v2.context = test.definedFunctions()[1];
    v3.type = child_y; v3.context = test.definedFunctions()[2];
    v4.type = child_z; v4.context = test.definedFunctions()[3];
    list<NewCallSummary::NewCall> expect({ v1, v2, v3, v4 });

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
            X x1;
            X x2;
            constructor() public {
                x1 = new X();
                x2 = new X();
            }
        }
        contract Z {
            X x1;
            Y y1;
            Y y2;
            constructor() public {
                x1 = new X();
                y1 = new Y();
                y2 = new Y();
            }
        }
        contract Q {
            X x1;
            Y y1;
            constructor() public {
                x1 = new X();
                y1 = new Y();
            }
        }
        contract R {
            Q q1;
            Z z1;
            constructor() public {
                q1 = new Q();
                z1 = new Z();
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
            X x;
            function f() public {
                x = new X();
            }
        }
        contract Z {
            X x;
            function g() public {
                x = new X();
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

BOOST_AUTO_TEST_CASE(family_analysis_test)
{
    char const* text = R"(
        contract X {}
        contract Y {
            X x1;
            X x2;
            constructor() public {
                x1 = new X();
                x2 = new X();
            }
        }
        contract Z {
            X x1;
            X x2;
            Y y1;
            constructor() public {
                x1 = new X();
                x2 = new X();
                y1 = new Y();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* x = retrieveContractByName(unit, "X");
    auto const* y = retrieveContractByName(unit, "Y");
    auto const* z = retrieveContractByName(unit, "Z");

    NewCallGraph graph;
    graph.record(unit);
    graph.finalize();

    BOOST_CHECK(graph.children_of(x).empty());
    BOOST_CHECK_EQUAL(graph.children_of(y).size(), 2);
    BOOST_CHECK_EQUAL(graph.children_of(z).size(), 3);
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
