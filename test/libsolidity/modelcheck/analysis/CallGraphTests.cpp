/**
 * Tests for libsolidity/modelcheck/analysis/CallGraph.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/CallGraph.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
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
    Analysis_CallGraphTests, ::dev::solidity::test::AnalysisFramework
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

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
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

    BOOST_CHECK_EQUAL(func_a_f->name(), "f");
    BOOST_CHECK_EQUAL(func_a_g->name(), "g");
    BOOST_CHECK_EQUAL(func_a_h->name(), "h");
    BOOST_CHECK(func_b->isConstructor());
    BOOST_CHECK_EQUAL(func_b_f->name(), "f");
    BOOST_CHECK_EQUAL(func_b_g->name(), "g");
    BOOST_CHECK_EQUAL(func_b_h->name(), "h");
    BOOST_CHECK(func_c->isConstructor());
    BOOST_CHECK_EQUAL(func_c_f->name(), "f");
    BOOST_CHECK_EQUAL(func_c_g->name(), "g");
    BOOST_CHECK_EQUAL(func_c_h->name(), "h");

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

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
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

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
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

    BOOST_CHECK_EQUAL(func_b_g->name(), "g");
    BOOST_CHECK_EQUAL(func_a_f->name(), "f");

    vector<ContractDefinition const*> model({ ctrt_b });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 3);
    BOOST_CHECK(call_graph->has_vertex(func_a_f));
    BOOST_CHECK(call_graph->has_vertex(ctrt_b->definedFunctions()[0]));
    BOOST_CHECK(call_graph->has_vertex(func_b_g));

    auto const& labels = call_graph->label_of(func_b_g, func_a_f);
    BOOST_CHECK_EQUAL(labels.size(), 1);
    BOOST_CHECK(labels.find(CallTypes::Super) != labels.end());
}

BOOST_AUTO_TEST_CASE(call_graph_with_fallback_internals)
{
    char const* text = R"(
        contract A {
            int _x = 0;
            function f() internal { _x += 1; }
            function() external { f(); }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");

    auto func_a_f = ctrt_a->definedFunctions()[0];
    auto func_a_fallback = ctrt_a->definedFunctions()[1];

    BOOST_CHECK_EQUAL(func_a_f->name(), "f");
    BOOST_CHECK_EQUAL(func_a_fallback->name(), "");

    vector<ContractDefinition const*> model({ ctrt_a });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 2);
    BOOST_CHECK(call_graph->has_vertex(func_a_f));
    BOOST_CHECK(call_graph->has_vertex(func_a_fallback));
    BOOST_CHECK(call_graph->has_edge(func_a_fallback, func_a_f));
    BOOST_CHECK(call_graph->label_of(func_a_fallback, func_a_f).empty());
}

BOOST_AUTO_TEST_CASE(executable_code)
{
    char const* text = R"(
        contract X {
            function f() public pure { }
            function g() public pure { }
            function h() public pure { }
        }
        contract Y is X {
            function f() public pure { }
            function g() public pure { }
            function q() public pure { }
            function r() public pure { }
        }
        contract Z is Y {
            function f() public pure { super.f(); }
            function g() public pure { super.g(); }
            function h() public pure { super.h(); }
            function q() public pure { super.q(); }
            function r() public pure { super.r(); }
        }
        contract Q {
            function f() private pure {}
            function g() private pure {}
            function h() public pure { g(); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(unit, "X");
    auto const* ctrt_y = retrieveContractByName(unit, "Y");
    auto const* ctrt_z = retrieveContractByName(unit, "Z");
    auto const* ctrt_q = retrieveContractByName(unit, "Q");

    vector<ContractDefinition const*> model_all({ ctrt_x, ctrt_y, ctrt_z, ctrt_q });
    auto alloc_graph = make_shared<AllocationGraph>(model_all);
    auto flat_model = make_shared<FlatModel>(model_all, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    vector<ContractDefinition const*> model_x({ ctrt_x });
    vector<ContractDefinition const*> model_y({ ctrt_y });
    vector<ContractDefinition const*> model_z({ ctrt_z });
    vector<ContractDefinition const*> model_q({ ctrt_z, ctrt_q });
    auto flat_model_x = make_shared<FlatModel>(model_x, *alloc_graph);
    auto flat_model_y = make_shared<FlatModel>(model_y, *alloc_graph);
    auto flat_model_z = make_shared<FlatModel>(model_z, *alloc_graph);
    auto flat_model_q = make_shared<FlatModel>(model_q, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model_x->view().size(), 1);
    BOOST_CHECK_EQUAL(flat_model_y->view().size(), 1);
    BOOST_CHECK_EQUAL(flat_model_z->view().size(), 1);
    BOOST_CHECK_EQUAL(flat_model_q->view().size(), 2);

    CallGraph graph_x(r, flat_model_x);
    CallGraph graph_y(r, flat_model_y);
    CallGraph graph_z(r, flat_model_z);
    CallGraph graph_q(r, flat_model_q);

    BOOST_CHECK_EQUAL(graph_x.executed_code().size(), 3);
    BOOST_CHECK_EQUAL(graph_y.executed_code().size(), 5);
    BOOST_CHECK_EQUAL(graph_z.executed_code().size(), 10);
    BOOST_CHECK_EQUAL(graph_q.executed_code().size(), 12);
}

BOOST_AUTO_TEST_CASE(linearizes_direct_super_calls)
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

    vector<ContractDefinition const*> model({ ctrt_r });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);
    BOOST_CHECK_NE(flat_model->get(*ctrt_y).get(), nullptr);

    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 3);

    auto func_r_f = ctrt_r->definedFunctions()[0];
    auto func_z_f = ctrt_z->definedFunctions()[0];
    auto func_y_f = ctrt_y->definedFunctions()[0];

    BOOST_CHECK_EQUAL(func_r_f->name(), "f");
    BOOST_CHECK_EQUAL(func_z_f->name(), "f");
    BOOST_CHECK_EQUAL(func_y_f->name(), "f");

    auto scope = flat_model->get(*ctrt_r);
    auto supers = graph.super_calls(*scope, *func_r_f);
    BOOST_CHECK_EQUAL(supers.size(), 3);

    BOOST_CHECK(supers.find(func_r_f) != supers.end());
    BOOST_CHECK(supers.find(func_z_f) != supers.end());
    BOOST_CHECK(supers.find(func_y_f) != supers.end());
}

BOOST_AUTO_TEST_CASE(linearizes_mixed_super_calls)
{
    char const* text = R"(
        contract X {
            function f() public pure returns (int a) { a = 5; }
            function g() public pure returns (int a) { a = 10; }
        }
        contract Y is X {
            function f() public pure returns (int a) { a = 5; }
            function g() public pure returns (int a) { a = 5; }
            function h() public pure {
                assert(super.f() != super.g());
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(unit, "X");
    auto const* ctrt_y = retrieveContractByName(unit, "Y");

    vector<ContractDefinition const*> model({ ctrt_x, ctrt_y });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 2);
    BOOST_CHECK_NE(flat_model->get(*ctrt_x).get(), nullptr);
    BOOST_CHECK_NE(flat_model->get(*ctrt_y).get(), nullptr);

    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 5);

    auto flat_x = flat_model->get(*ctrt_x);
    auto flat_y = flat_model->get(*ctrt_y);

    auto x_func_f = ctrt_x->definedFunctions()[0];
    auto x_func_g = ctrt_x->definedFunctions()[1];
    auto y_func_f = ctrt_y->definedFunctions()[0];
    auto y_func_g = ctrt_y->definedFunctions()[1];
    auto y_func_h = ctrt_y->definedFunctions()[2];

    BOOST_CHECK_EQUAL(x_func_f->name(), "f");
    BOOST_CHECK_EQUAL(x_func_g->name(), "g");
    BOOST_CHECK_EQUAL(y_func_f->name(), "f");
    BOOST_CHECK_EQUAL(y_func_g->name(), "g");
    BOOST_CHECK_EQUAL(y_func_h->name(), "h");

    BOOST_CHECK_EQUAL(graph.super_calls(*flat_x, *x_func_f).size(), 1);
    BOOST_CHECK_EQUAL(graph.super_calls(*flat_x, *x_func_g).size(), 1);
    BOOST_CHECK_EQUAL(graph.super_calls(*flat_y, *y_func_f).size(), 2);
    BOOST_CHECK_EQUAL(graph.super_calls(*flat_y, *y_func_g).size(), 2);
    BOOST_CHECK_EQUAL(graph.super_calls(*flat_y, *y_func_h).size(), 1);
}

BOOST_AUTO_TEST_CASE(linearizes_indirect_super_calls)
{
    char const* text = R"(
        contract X {
            function f() public pure {}
            function g() public pure {}
        }
        contract Y is X {
            function f() public pure {}
            function g() public pure { super.f(); }
        }
        contract Z is Y {
            function f() public pure {}
            function g() public pure { super.g(); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_y = retrieveContractByName(unit, "Y");
    auto const* ctrt_z = retrieveContractByName(unit, "Z");

    vector<ContractDefinition const*> model({ ctrt_z });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);
    BOOST_CHECK_NE(flat_model->get(*ctrt_z).get(), nullptr);

    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 4);

    auto func_y_f = ctrt_y->definedFunctions()[0];
    auto func_z_f = ctrt_z->definedFunctions()[0];

    BOOST_CHECK_EQUAL(func_y_f->name(), "f");
    BOOST_CHECK_EQUAL(func_z_f->name(), "f");

    auto scope = flat_model->get(*ctrt_z);
    auto supers = graph.super_calls(*scope, *func_z_f);
    BOOST_CHECK_EQUAL(supers.size(), 2);
    BOOST_CHECK(supers.find(func_y_f) == supers.end());
}

BOOST_AUTO_TEST_CASE(internals)
{
    char const* text = R"(
        contract X {
            function f() internal pure {}
            function g() internal pure {}
        }
        contract Y is X {
            function f() internal pure {}
            function g() internal pure { super.f(); }
        }
        contract Z is Y {
            function p() public pure {}
            function q() public pure { super.g(); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(unit, "X");
    auto const* ctrt_y = retrieveContractByName(unit, "Y");
    auto const* ctrt_z = retrieveContractByName(unit, "Z");

    vector<ContractDefinition const*> model({ ctrt_z });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);
    BOOST_CHECK_NE(flat_model->get(*ctrt_z).get(), nullptr);

    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 4);

    auto func_x_f = ctrt_x->definedFunctions()[0];
    auto func_y_g = ctrt_y->definedFunctions()[1];

    BOOST_CHECK_EQUAL(func_x_f->name(), "f");
    BOOST_CHECK_EQUAL(func_y_g->name(), "g");

    auto internals = graph.internals(*flat_model->get(*ctrt_z));
    BOOST_CHECK_EQUAL(internals.size(), 2);
    BOOST_CHECK(internals.find(func_x_f) != internals.end());
    BOOST_CHECK(internals.find(func_y_g) != internals.end());
}

BOOST_AUTO_TEST_CASE(lib_calls)
{
    char const* text = R"(
        library X {
            function incr(uint256 _x) internal pure { _x += 1; }
        }
        contract Y {
            using X for uint256;
            function f(uint256 _x) public pure { _x.incr(); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(unit, "X");
    auto const* ctrt_y = retrieveContractByName(unit, "Y");

    vector<ContractDefinition const*> model({ ctrt_y });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK_EQUAL(call_graph->vertices().size(), 2);

    auto func_x_incr = ctrt_x->definedFunctions()[0];
    auto func_y_f = ctrt_y->definedFunctions()[0];

    BOOST_CHECK_EQUAL(func_x_incr->name(), "incr");
    BOOST_CHECK_EQUAL(func_y_f->name(), "f");

    auto neighbours = call_graph->neighbours(func_y_f);
    BOOST_CHECK_EQUAL(neighbours.size(), 1);
    if (!neighbours.empty())
    {
        BOOST_CHECK(neighbours.find(func_x_incr) != neighbours.end());
    }

    auto labels = call_graph->label_of(func_y_f, func_x_incr);
    BOOST_CHECK_EQUAL(labels.size(), 1);
    BOOST_CHECK(labels.find(CallTypes::Library) != labels.end());
}

BOOST_AUTO_TEST_CASE(super_is_temporary)
{
    char const* text = R"(
        contract X {
            function f() public pure {}
            function g() public pure { f(); }
        }
        contract Y is X {
            function f() public pure {}
            function g() public pure { super.g(); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_y = retrieveContractByName(unit, "Y");

    vector<ContractDefinition const*> model({ ctrt_y });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);
    BOOST_CHECK_NE(flat_model->get(*ctrt_y).get(), nullptr);

    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 3);

    auto func_y_f = ctrt_y->definedFunctions()[0];
    BOOST_CHECK_EQUAL(func_y_f->name(), "f");

    auto flat_y = flat_model->get(*ctrt_y);
    BOOST_CHECK_EQUAL(graph.super_calls(*flat_y, *func_y_f).size(), 1);
}

BOOST_AUTO_TEST_CASE(internals_from_fallback)
{
    char const* text = R"(
        contract A {
            int _x = 0;
            function f() internal { _x += 1; }
            function() external { f(); }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");

    auto func_a_f = ctrt_a->definedFunctions()[0];
    auto func_a_fallback = ctrt_a->definedFunctions()[1];

    BOOST_CHECK_EQUAL(func_a_f->name(), "f");
    BOOST_CHECK_EQUAL(func_a_fallback->name(), "");

    vector<ContractDefinition const*> model({ ctrt_a });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraph graph(r, flat_model);
    auto flat_contract = flat_model->get(*ctrt_a);
    BOOST_CHECK_EQUAL(graph.internals(*flat_contract).size(), 1);
    BOOST_CHECK_EQUAL(graph.super_calls(*flat_contract, *func_a_f).size(), 1);
}

BOOST_AUTO_TEST_CASE(can_downcast_rvs)
{
    char const* text = R"(
        contract A {
            function f() public view {}
        }
        contract B is A {
            function f() public view {}
        }
        contract C {
            A a;
            constructor () public {
                a = new B();
            }
            function get() public returns(A) {
                return a;
            }
        }
        contract D {
            C c;
            constructor() public {
                c = new C();
            }
            function f() public {
                c.get().f();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_d = retrieveContractByName(unit, "D");

    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");

    auto func_a_f = ctrt_a->definedFunctions()[0];
    auto func_b_f = ctrt_b->definedFunctions()[0];
    auto func_d_f = ctrt_d->definedFunctions()[1];

    BOOST_CHECK_EQUAL(func_a_f->name(), "f");
    BOOST_CHECK_EQUAL(func_b_f->name(), "f");
    BOOST_CHECK_EQUAL(func_d_f->name(), "f");

    vector<ContractDefinition const*> model({ ctrt_d });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);

    CallGraphBuilder builder(r);
    auto call_graph = builder.build(flat_model);
    BOOST_CHECK(!call_graph->has_edge(func_d_f, func_a_f));
    BOOST_CHECK(call_graph->has_edge(func_d_f, func_b_f));
}

BOOST_AUTO_TEST_CASE(libs_are_not_internal)
{
    char const* text = R"(
        library X {
            function incr(uint256 _x) internal pure { _x += 1; }
        }
        contract Y {
            using X for uint256;
            function g(uint256 _x) internal pure { _x.incr(); }
            function f(uint256 _x) public pure { g(_x); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(unit, "X");
    auto const* ctrt_y = retrieveContractByName(unit, "Y");

    vector<ContractDefinition const*> model({ ctrt_y });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 3);

    auto func_x_incr = ctrt_x->definedFunctions()[0];
    BOOST_CHECK_EQUAL(func_x_incr->name(), "incr");

    auto const& scope = (*flat_model->get(*ctrt_y));
    BOOST_CHECK_EQUAL(graph.internals(scope).size(), 1);
    BOOST_CHECK(graph.super_calls(scope, *func_x_incr).empty());
}

BOOST_AUTO_TEST_CASE(transfer_calls)
{
    char const* text = R"(
        contract X {
            function f(address payable _x) public {
                _x.transfer(5);
                require(_x.send(5));
                _x.call.value(5)("");
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const* ctrt_x = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt_x });
    auto alloc_graph = make_shared<AllocationGraph>(model);

    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);

    auto r = make_shared<ContractExpressionAnalyzer>(*flat_model, alloc_graph);
    CallGraph graph(r, flat_model);
    BOOST_CHECK_EQUAL(graph.executed_code().size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
