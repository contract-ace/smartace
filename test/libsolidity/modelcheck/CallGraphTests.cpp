/**
 * Test suite targeting CallGraph manipulation.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/CallGraph.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <cstdint>

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
    CallGraphTests,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(digraph)
{
    Digraph<uint8_t> g_1;
    Digraph<uint8_t> g_2;
    Digraph<uint8_t> g_3;
    BOOST_CHECK_EQUAL(g_3.vertices().size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(2).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(4).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(8).size(), 0);

    g_2.add_vertex(2);
    g_3.add_vertex(2);
    BOOST_CHECK_EQUAL(g_3.vertices().size(), 1);
    BOOST_CHECK_EQUAL(g_3.neighbours(2).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(4).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(8).size(), 0);

    g_3.add_vertex(4);
    g_2.add_vertex(4);
    BOOST_CHECK_EQUAL(g_3.vertices().size(), 2);
    BOOST_CHECK_EQUAL(g_3.neighbours(2).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(4).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(8).size(), 0);

    g_2.add_vertex(8);
    g_3.add_vertex(8);
    BOOST_CHECK_EQUAL(g_3.vertices().size(), 3);
    BOOST_CHECK_EQUAL(g_3.neighbours(2).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(4).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(8).size(), 0);

    g_3.add_edge(2, 8);
    BOOST_CHECK_EQUAL(g_3.neighbours(2).size(), 1);
    BOOST_CHECK_EQUAL(g_3.neighbours(4).size(), 0);
    BOOST_CHECK_EQUAL(g_3.neighbours(8).size(), 0);

    g_3.add_edge(4, 8);
    BOOST_CHECK_EQUAL(g_3.neighbours(2).size(), 1);
    BOOST_CHECK_EQUAL(g_3.neighbours(4).size(), 1);
    BOOST_CHECK_EQUAL(g_3.neighbours(8).size(), 0);

    for (uint8_t v_1 = 0; v_1 < UINT8_MAX; ++v_1)
    {
        BOOST_CHECK(!g_1.has_vertex(v_1));
        if (v_1 == 2 || v_1 == 4 || v_1 == 8)
        {
            BOOST_CHECK(g_2.has_vertex(v_1));
            BOOST_CHECK(g_3.has_vertex(v_1));
        }
        else
        {
            BOOST_CHECK(!g_2.has_vertex(v_1));
            BOOST_CHECK(!g_3.has_vertex(v_1));
        }

        for (uint8_t v_2 = 0; v_2 < UINT8_MAX; ++v_2)
        {
            BOOST_CHECK(!g_1.has_edge(v_1, v_2));
            BOOST_CHECK(!g_2.has_edge(v_1, v_2));
            if ((v_1 == 2 || v_1 == 4) && v_2 == 8)
            {
                BOOST_CHECK(g_3.has_edge(v_1, v_2));
            }
            else
            {
                BOOST_CHECK(!g_3.has_edge(v_1, v_2));
            }
        }
    }

    BOOST_CHECK_THROW(g_1.add_edge(1, 2), invalid_argument);
    BOOST_CHECK_THROW(g_2.add_edge(1, 2), invalid_argument);
    BOOST_CHECK_THROW(g_2.add_edge(2, 1), invalid_argument);
}

BOOST_AUTO_TEST_CASE(labels)
{
    LabeledDigraph<uint8_t, uint8_t> graph;

    graph.add_vertex(1);
    graph.add_vertex(2);
    graph.add_vertex(3);
    
    graph.add_edge(1, 2);
    BOOST_CHECK(graph.label_of(1, 2).empty());
    BOOST_CHECK(graph.label_of(1, 3).empty());

    graph.label_edge(1, 2, 5);
    BOOST_CHECK_EQUAL(graph.label_of(1, 2).size(), 1);
    BOOST_CHECK(graph.label_of(1, 3).empty());

    graph.label_edge(1, 2, 6);
    BOOST_CHECK_EQUAL(graph.label_of(1, 2).size(), 2);
    BOOST_CHECK(graph.label_of(1, 3).empty());
}

BOOST_AUTO_TEST_CASE(call_graph_builder)
{
    char const* text = R"(
        contract A {
            function f() external pure {}
            function g() external pure {}
            function h() external pure {}
            function p() internal pure {}
        }
        contract B {
            A a;
            constructor() public { a = new A(); }
            function f() internal pure { f(); }
            function g() public { a.f(); a.f(); }
            function h() external {
                g();
                f();
                a.g();
            }
            function p() internal pure {}
        }
        contract C {
            A a;
            B b;
            constructor() public {
                a = new A();
                b = new B();
            }
            function f() external pure {}
            function g() external { a.h(); }
            function h() external {
                b.g();
                b.h();
            }
            function p() internal pure {}
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");
    auto ctrt_c = retrieveContractByName(unit, "C");

    vector<ContractDefinition const*> model({ ctrt_c });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 3);

    CallGraphBuilder builder(alloc_graph);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 11);

    auto func_a_f = ctrt_a->definedFunctions()[0];
    auto func_a_g = ctrt_a->definedFunctions()[1];
    auto func_a_h = ctrt_a->definedFunctions()[2];
    auto func_b = ctrt_b->definedFunctions()[0];
    auto func_b_f = ctrt_b->definedFunctions()[1];
    auto func_b_g = ctrt_b->definedFunctions()[2];
    auto func_b_h = ctrt_b->definedFunctions()[3];
    auto func_c = ctrt_c->definedFunctions()[0];
    auto func_c_f = ctrt_c->definedFunctions()[1];
    auto func_c_g = ctrt_c->definedFunctions()[2];
    auto func_c_h = ctrt_c->definedFunctions()[3];
    BOOST_CHECK(call_graph->has_vertex(func_a_f));
    BOOST_CHECK(call_graph->has_vertex(func_a_g));
    BOOST_CHECK(call_graph->has_vertex(func_a_h));
    BOOST_CHECK(call_graph->has_vertex(func_b));
    BOOST_CHECK(call_graph->has_vertex(func_b_f));
    BOOST_CHECK(call_graph->has_vertex(func_b_g));
    BOOST_CHECK(call_graph->has_vertex(func_b_h));
    BOOST_CHECK(call_graph->has_vertex(func_c));
    BOOST_CHECK(call_graph->has_vertex(func_c_f));
    BOOST_CHECK(call_graph->has_vertex(func_c_g));
    BOOST_CHECK(call_graph->has_vertex(func_c_h));

    auto func_a_p = ctrt_a->definedFunctions()[3];
    auto func_b_p = ctrt_b->definedFunctions()[4];
    auto func_c_p = ctrt_c->definedFunctions()[4];
    BOOST_CHECK(!call_graph->has_vertex(func_a_p));
    BOOST_CHECK(!call_graph->has_vertex(func_b_p));
    BOOST_CHECK(!call_graph->has_vertex(func_c_p));

    BOOST_CHECK_EQUAL(call_graph->neighbours(func_a_f).size(), 0);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_a_g).size(), 0);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_a_h).size(), 0);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_b).size(), 0);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_b_f).size(), 1);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_b_g).size(), 1);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_b_h).size(), 3);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_c).size(), 1);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_c_f).size(), 0);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_c_g).size(), 1);
    BOOST_CHECK_EQUAL(call_graph->neighbours(func_c_h).size(), 2);

    BOOST_CHECK(call_graph->has_edge(func_b_f, func_b_f));
    BOOST_CHECK(call_graph->label_of(func_b_f, func_b_f).empty());
    BOOST_CHECK(call_graph->has_edge(func_b_g, func_a_f));
    BOOST_CHECK_EQUAL(call_graph->label_of(func_b_g, func_a_f).size(), 1);
    BOOST_CHECK(call_graph->has_edge(func_b_h, func_a_g));
    BOOST_CHECK_EQUAL(call_graph->label_of(func_b_h, func_a_g).size(), 1);
    BOOST_CHECK(call_graph->has_edge(func_b_h, func_b_f));
    BOOST_CHECK(call_graph->label_of(func_b_h, func_b_f).empty());
    BOOST_CHECK(call_graph->has_edge(func_b_h, func_b_g));
    BOOST_CHECK(call_graph->label_of(func_b_h, func_b_g).empty());
    BOOST_CHECK(call_graph->has_edge(func_c, func_b));
    BOOST_CHECK_EQUAL(call_graph->label_of(func_c, func_b).size(), 1);
    BOOST_CHECK(call_graph->has_edge(func_c_g, func_a_h));
    BOOST_CHECK_EQUAL(call_graph->label_of(func_c_g, func_a_h).size(), 1);
    BOOST_CHECK(call_graph->has_edge(func_c_h, func_b_g));
    BOOST_CHECK_EQUAL(call_graph->label_of(func_c_h, func_b_g).size(), 1);
    BOOST_CHECK(call_graph->has_edge(func_c_h, func_b_h));
    BOOST_CHECK_EQUAL(call_graph->label_of(func_c_h, func_b_h).size(), 1);

    auto const& ext_label = call_graph->label_of(func_b_g, func_a_f);
    BOOST_CHECK(ext_label.find(CallTypes::External) != ext_label.end());

    auto const& alloc_label = call_graph->label_of(func_c, func_b);
    BOOST_CHECK(alloc_label.find(CallTypes::Alloc) != alloc_label.end());
}

BOOST_AUTO_TEST_CASE(call_graph_build_with_inheritance)
{
    char const* text = R"(
        contract A {
            function f() public pure {}
            function g() public pure { f(); }
        }
        contract B is A {
            function f() public pure {}
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_b = retrieveContractByName(unit, "B");

    vector<ContractDefinition const*> model({ ctrt_b });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    CallGraphBuilder builder(alloc_graph);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 2);
}

BOOST_AUTO_TEST_CASE(call_graph_build_with_downcasting)
{
    char const* text = R"(
        contract A {
            function f() public pure {}
        }
        contract B is A {
            function f() public pure {}
        }
        contract C {
            A a;
            constructor() public { a = new B(); }
            function g() public { a.f(); }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_c = retrieveContractByName(unit, "C");

    vector<ContractDefinition const*> model({ ctrt_c });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 2);

    CallGraphBuilder builder(alloc_graph);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 3);
}

BOOST_AUTO_TEST_CASE(call_graph_with_super)
{
    char const* text = R"(
        contract A {
            function f() public pure {}
            function g() public pure {}
        }
        contract B is A {
            function f() public pure {}
            function g() public pure { super.f(); }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");

    auto func_b_g = ctrt_b->definedFunctions()[1];
    auto func_a_f = ctrt_a->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt_b });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    CallGraphBuilder builder(alloc_graph);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 3);
    BOOST_CHECK(call_graph->has_vertex(func_a_f));
    BOOST_CHECK(call_graph->has_vertex(ctrt_b->definedFunctions()[0]));
    BOOST_CHECK(call_graph->has_vertex(func_b_g));

    auto const& labels = call_graph->label_of(func_b_g, func_a_f);
    BOOST_CHECK_EQUAL(labels.size(), 1);
    BOOST_CHECK(labels.find(CallTypes::Super) != labels.end());
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
