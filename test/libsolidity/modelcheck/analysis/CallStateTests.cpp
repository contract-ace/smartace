/**
 * Tests for libsolidity/modelcheck/analysis/CallState.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/analysis/CallState.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Primitives.h>

#include <vector>

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
    Analysis_CallStateTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(add_to_primitives)
{
    char const* text = R"(
        contract A {
            address m_sender;
            address m_origin;
            uint256 m_now;
            uint256 m_blocknum;
            function f() public payable {
                m_sender = msg.sender;
                m_origin = tx.origin;
                m_now = now;
                m_blocknum = block.number;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);

    CallState call_state(call_graph, false);
    PrimitiveTypeGenerator primitive_generator;

    call_state.register_primitives(primitive_generator);
    BOOST_CHECK(primitive_generator.found_uint(32));
    BOOST_CHECK(primitive_generator.found_address());
    BOOST_CHECK(primitive_generator.found_bool());
}

BOOST_AUTO_TEST_CASE(detects_payments)
{
    char const* text = R"(
        contract A {
            function f(address payable _a) public {
                require(_a.send(5));
            }
        }
        contract B {
            function f(address payable _a) public {
                _a.transfer(5);
            }
        }
        contract C {
            function f() public payable {}
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(ast, "A");
    auto ctrt_b = retrieveContractByName(ast, "B");
    auto ctrt_c = retrieveContractByName(ast, "C");

    vector<ContractDefinition const*> model_a({ ctrt_a });
    auto alloc_graph_a = make_shared<AllocationGraph>(model_a);
    auto flat_model_a = make_shared<FlatModel>(model_a, *alloc_graph_a);
    auto r_a = make_shared<ContractExpressionAnalyzer>(flat_model_a, alloc_graph_a);
    CallGraph call_graph_a(r_a, flat_model_a);
    CallState call_state_a(call_graph_a, false);
    BOOST_CHECK(!call_state_a.uses_pay());
    BOOST_CHECK(call_state_a.uses_send());
    BOOST_CHECK(!call_state_a.uses_transfer());

    vector<ContractDefinition const*> model_b({ ctrt_b });
    auto alloc_graph_b = make_shared<AllocationGraph>(model_b);
    auto flat_model_b = make_shared<FlatModel>(model_b, *alloc_graph_b);
    auto r_b = make_shared<ContractExpressionAnalyzer>(flat_model_b, alloc_graph_b);
    CallGraph call_graph_b(r_b, flat_model_b);
    CallState call_state_b(call_graph_b, false);
    BOOST_CHECK(!call_state_b.uses_pay());
    BOOST_CHECK(!call_state_b.uses_send());
    BOOST_CHECK(call_state_b.uses_transfer());

    vector<ContractDefinition const*> model_c({ ctrt_c });
    auto alloc_graph_c = make_shared<AllocationGraph>(model_c);
    auto flat_model_c = make_shared<FlatModel>(model_c, *alloc_graph_c);
    auto r_c = make_shared<ContractExpressionAnalyzer>(flat_model_c, alloc_graph_c);
    CallGraph call_graph_c(r_a, flat_model_c);
    CallState call_state_c(call_graph_c, false);
    BOOST_CHECK(call_state_c.uses_pay());
    BOOST_CHECK(!call_state_c.uses_send());
    BOOST_CHECK(!call_state_c.uses_transfer());
}

BOOST_AUTO_TEST_CASE(escalate_reqs)
{
    char const* text = R"(
        contract X {}
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt_x = retrieveContractByName(ast, "X");

    vector<ContractDefinition const*> model({ ctrt_x });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);

    CallState call_state_a(call_graph, false);
    BOOST_CHECK(!call_state_a.escalate_requires());
    for (auto param : call_state_a.order())
    {
        BOOST_CHECK(param.field != CallStateUtilities::Field::ReqFail);
    }

    bool req_fail_found = false;
    CallState call_state_b(call_graph, true);
    BOOST_CHECK(call_state_b.escalate_requires());
    for (auto param : call_state_a.order())
    {
        req_fail_found = true;
    }
    BOOST_CHECK(req_fail_found);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
