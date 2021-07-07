/**
 * Tests for libsolidity/modelcheck/analysis/TightBundle.
 * 
 * @date 2021
 */

#include <libsolidity/modelcheck/analysis/TightBundle.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <set>

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
    Analysis_TightBundleTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(contract)
{
    char const* text = R"(
		contract A {}
        contract B is A {
            A a;
            A b;
            constructor() public { a = new A(); b = new A(); }
        }
        contract C is A {
            A a;
            constructor() public { a = new B(); }
        }
        contract D {}
        contract E {
            A a;
            A b;
            constructor() public { a = new A(); b = new C(); }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");
    auto ctrt_d = retrieveContractByName(unit, "D");
    auto ctrt_e = retrieveContractByName(unit, "E");

    auto store = make_shared<StructureStore>();
    vector<ContractDefinition const*> model({ ctrt_d, ctrt_e });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    CallState call_state(call_graph, false);

    BOOST_CHECK_EQUAL(flat_model->view().size(), 5);

    auto flat_a = flat_model->get(*ctrt_a);
    auto flat_b = flat_model->get(*ctrt_b);

    BOOST_CHECK_NE(flat_a, nullptr);
    BOOST_CHECK_NE(flat_b, nullptr);
    
    size_t size = 0;
    BundleContract const bundle_contract(
        *flat_model, call_state, false, flat_b, size, ""
    );

    BOOST_CHECK_EQUAL(size, 3);
    BOOST_CHECK_EQUAL(bundle_contract.address(), 1);
    BOOST_CHECK_EQUAL(bundle_contract.var(), "");
    BOOST_CHECK_EQUAL(bundle_contract.details().get(), flat_b.get());
    BOOST_CHECK_EQUAL(bundle_contract.children().size(), 2);
    if (bundle_contract.children().size() >= 1)
    {
        auto const& child = bundle_contract.children()[0];
        BOOST_CHECK_EQUAL(child->address(), 2);
        BOOST_CHECK_EQUAL(child->var(), "a");
        BOOST_CHECK_EQUAL(child->details().get(), flat_a.get());
        BOOST_CHECK_EQUAL(child->children().size(), 0);
    }
    if (bundle_contract.children().size() >= 2)
    {
        auto const& child = bundle_contract.children()[1];
        BOOST_CHECK_EQUAL(child->address(), 3);
        BOOST_CHECK_EQUAL(child->var(), "b");
        BOOST_CHECK_EQUAL(child->details().get(), flat_a.get());
        BOOST_CHECK_EQUAL(child->children().size(), 0);
    }
}

BOOST_AUTO_TEST_CASE(full)
{
    char const* text = R"(
		contract A {}
        contract B is A {
            A a;
            A b;
            constructor() public { a = new A(); b = new A(); }
        }
        contract C is A {
            A a;
            constructor() public { a = new B(); }
        }
        contract D {}
        contract E {
            A a;
            A b;
            constructor() public { a = new A(); b = new C(); }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_d = retrieveContractByName(unit, "D");
    auto ctrt_e = retrieveContractByName(unit, "E");

    auto store = make_shared<StructureStore>();
    vector<ContractDefinition const*> model({ ctrt_d, ctrt_e });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    CallState call_state(call_graph, false);

    BOOST_CHECK_EQUAL(flat_model->view().size(), 5);

    auto flat_d = flat_model->get(*ctrt_d);
    auto flat_e = flat_model->get(*ctrt_e);

    BOOST_CHECK_NE(flat_d, nullptr);
    BOOST_CHECK_NE(flat_e, nullptr);
    
    TightBundleModel tight_model(*flat_model, call_state, false);

    BOOST_CHECK_EQUAL(tight_model.size(), 7);
    BOOST_CHECK_EQUAL(tight_model.view().size(), 2);
    if (tight_model.view().size() >= 1)
    {
        auto tight_contract = tight_model.view()[0];
        BOOST_CHECK_EQUAL(tight_contract->details().get(), flat_d.get());
    }
    if (tight_model.view().size() >= 2)
    {
        auto tight_contract = tight_model.view()[1];
        BOOST_CHECK_EQUAL(tight_contract->details().get(), flat_e.get());
    }
}

BOOST_AUTO_TEST_CASE(fallbacks)
{
    char const* text = R"(
		contract A {
            function() external {}
        }
        contract B {
            function f() public payable {
                msg.sender.send(msg.value);
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");

    auto store = make_shared<StructureStore>();
    vector<ContractDefinition const*> model({ ctrt_a, ctrt_b });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    CallState call_state(call_graph, false);

    BOOST_CHECK_EQUAL(flat_model->view().size(), 2);

    auto flat_a = flat_model->get(*ctrt_a);
    auto flat_b = flat_model->get(*ctrt_b);

    TightBundleModel tight_model_false(*flat_model, call_state, false);
    BOOST_CHECK_EQUAL(tight_model_false.view().size(), 2);
    if (tight_model_false.view().size() >= 1)
    {
        auto tight_contract = tight_model_false.view()[0];
        BOOST_CHECK(!tight_contract->can_fallback_through_send());
    }
    if (tight_model_false.view().size() >= 2)
    {
        auto tight_contract = tight_model_false.view()[1];
        BOOST_CHECK(!tight_contract->can_fallback_through_send());
    }
    
    TightBundleModel tight_model_true(*flat_model, call_state, true);
    BOOST_CHECK_EQUAL(tight_model_true.view().size(), 2);
    if (tight_model_true.view().size() >= 1)
    {
        auto tight_contract = tight_model_true.view()[0];
        BOOST_CHECK(tight_contract->can_fallback_through_send());
    }
    if (tight_model_true.view().size() >= 2)
    {
        auto tight_contract = tight_model_true.view()[1];
        BOOST_CHECK(!tight_contract->can_fallback_through_send());
    }
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
