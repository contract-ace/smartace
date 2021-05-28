/**
 * Tests for libsolidity/modelcheck/analysis/Library.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/analysis/Library.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <memory>
#include <string>

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
    Analysis_LibraryTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(library)
{
    char const* text = R"(
        library Lib {
            function f() public pure {}
            function g() public pure {}
            function h() public pure {}
        }
        contract A {
            function f() public pure {
                Lib.f();
                Lib.g();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    StructureStore store;
    vector<ContractDefinition const*> model({ ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    auto call_graph = make_shared<CallGraph>(r, flat_model);
    BOOST_CHECK_EQUAL(flat_model->view().size(), 1);
    BOOST_CHECK_EQUAL(flat_model->view().front()->interface().size(), 1);
    BOOST_CHECK_EQUAL(call_graph->executed_code().size(), 3);

    LibrarySummary summary(*call_graph, store);
    BOOST_CHECK_EQUAL(summary.view().size(), 1);
    BOOST_CHECK_EQUAL(summary.view().front()->functions().size(), 2);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}

