/**
 * Tests for libsolidity/modelcheck/analysis/AllocationSites.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

using namespace std;

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
    Analysis_AllocationSitesTests, ::dev::solidity::test::AnalysisFramework
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

    AllocationSummary app1(test1);
    auto const& actviolations1 = app1.violations();
    auto const& actchild1 = app1.children();

    BOOST_CHECK(actviolations1.empty());
    BOOST_CHECK(actchild1.empty());

    AllocationSummary app2(test2);
    auto const& actviolations2 = app2.violations();
    auto const& actchild2 = app2.children();

    AllocationSummary::NewCall c1, c2, c3, c4, c5, c6;
    c1.type = child_x;
    c2.type = child_y;
    c3.type = child_y;
    c4.type = child_z;
    c5.type = child_z;
    c6.type = child_z;
    AllocationSummary::CallGroup expchild2({ c1, c2, c3, c4, c5, c6 });

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

    AllocationSummary app3(test3);
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

    AllocationSummary app(test);
    BOOST_CHECK(app.children().empty());

    AllocationSummary::NewCall v1, v2, v3, v4;
    v1.type = child_q; v1.context = test.definedFunctions()[0];
    v2.type = child_x; v2.context = test.definedFunctions()[1];
    v3.type = child_y; v3.context = test.definedFunctions()[2];
    v4.type = child_z; v4.context = test.definedFunctions()[3];
    list<AllocationSummary::NewCall> expect({ v1, v2, v3, v4 });

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

    AllocationGraph graph({ child_r });

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

    auto child_x = retrieveContractByName(unit, "X");
    auto child_y = retrieveContractByName(unit, "Y");
    auto child_z = retrieveContractByName(unit, "Z");

    AllocationGraph graph({ child_y, child_z });

    AllocationSummary::NewCall v1, v2;
    v1.type = child_x; v1.context = child_y->definedFunctions()[0];
    v2.type = child_x; v2.context = child_z->definedFunctions()[0];
    list<AllocationSummary::NewCall> expect({v1, v2});

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

    AllocationGraph graph({ z });

    BOOST_CHECK(graph.children_of(x).empty());
    BOOST_CHECK_EQUAL(graph.children_of(y).size(), 2);
    BOOST_CHECK_EQUAL(graph.children_of(z).size(), 3);
}

BOOST_AUTO_TEST_CASE(upcast_at_callsite_test)
{
    char const* text = R"(
        contract X {}
        contract Y is X {}
        contract Test {
            X x1;
            X x2;
            Y y;
            constructor() public {
                x1 = new X();
                x2 = new Y();
                y = new Y();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto const* ctrt = retrieveContractByName(unit, "Test");

    AllocationGraph graph({ ctrt });

    for (auto var : ctrt->stateVariables())
    {
        if (var->name() == "x1")
        {
            auto const& derv = graph.specialize(*var);
            BOOST_CHECK_EQUAL(derv.name(), "X");
        }
        else if (var->name() == "x2")
        {
            auto const& derv = graph.specialize(*var);
            BOOST_CHECK_EQUAL(derv.name(), "Y");
        }
        else if (var->name() == "y")
        {
            auto const& derv = graph.specialize(*var);
            BOOST_CHECK_EQUAL(derv.name(), "Y");
        }
    }
}

BOOST_AUTO_TEST_CASE(indirect_internal_assignment)
{
    char const* text = R"(
        contract X {}
        contract Test1 {
            function good_internal() internal returns (X) {
                return new X();
            }
        }
        contract Test2 {
            X x;
            function bad_internal() internal returns (X) {
                x = new X();
                return new X();
            }
        }
        contract Test3 {
            X x;
            function good_internal() internal returns (X) {
                return new X();
            }
            constructor() public {
                x = good_internal();
            }
        }
        contract Test4 {
            X x;
            function good_internal() internal returns (X) {
                return new X();
            }
            function bad_func() public {
                x = good_internal();
            }
        }
        contract Test5 {
            X x;
            function bad_external() external {
                x = new X();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* test1 = retrieveContractByName(unit, "Test1");
    AllocationSummary summary1(*test1);
    BOOST_CHECK(summary1.violations().empty());

    auto const* test2 = retrieveContractByName(unit, "Test2");
    AllocationSummary summary2(*test2);
    BOOST_CHECK(!summary2.violations().empty());

    auto const* test3 = retrieveContractByName(unit, "Test3");
    AllocationSummary summary3(*test3);
    BOOST_CHECK(summary3.violations().empty());

    auto const* test4 = retrieveContractByName(unit, "Test4");
    AllocationSummary summary4(*test4);
    BOOST_CHECK(!summary4.violations().empty());

    auto const* test5 = retrieveContractByName(unit, "Test5");
    AllocationSummary summary5(*test5);
    BOOST_CHECK(!summary5.violations().empty());
}

BOOST_AUTO_TEST_CASE(specialization_by_inderection)
{
    char const* text = R"(
        contract X {}
        contract Y is X {}
        contract Test {
            X c1;
            X c2;
            constructor() public {
                c1 = f();
                c2 = g();
            }
            function f() internal returns (X) {
                return new Y();
            }
            function g() internal returns (X) {
                return new X();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto const* ctrt = retrieveContractByName(unit, "Test");

    AllocationGraph g({ ctrt });
    BOOST_CHECK_EQUAL(g.specialize(*ctrt->stateVariables()[0]).name(), "Y");
    BOOST_CHECK_EQUAL(g.specialize(*ctrt->stateVariables()[1]).name(), "X");
}

BOOST_AUTO_TEST_CASE(resolve_id)
{
    char const* text = R"(
        contract X {}
        contract Y {}
        contract Test {
            X x;
            Y y;
            constructor() public {
                x = new X();
                y = new Y();
            }
            function f() public view { x; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "Test");
    
    auto stmt = ctrt->definedFunctions()[0]->body().statements()[0];
    auto expr_stmt = dynamic_cast<ExpressionStatement const*>(stmt.get());
    auto assignment = dynamic_cast<Assignment const*>(&expr_stmt->expression());
    auto id = (&assignment->leftHandSide());

    AllocationGraph g({ ctrt });
    BOOST_CHECK_EQUAL(g.resolve(*id).name(), "X");
}

BOOST_AUTO_TEST_CASE(member_access_to_contracts)
{
    char const* text = R"(
        contract X {}
        contract Test {
            struct S { X x; }
            S s;
            constructor() public { s.x = new X(); }
            function f() public view { s.x; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto const* ctrt = retrieveContractByName(unit, "Test");

    auto struct_s = ctrt->definedStructs()[0];
    auto var_x = struct_s->members()[0].get();
    AllocationGraph g({ ctrt });
    BOOST_CHECK_EQUAL(g.specialize(*var_x).name(), "X");
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
