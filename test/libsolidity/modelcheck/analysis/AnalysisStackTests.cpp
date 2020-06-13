/**
 * Tests for libsolidity/modelcheck/analysis/AnalysisStack.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>

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
    Analysis_AnalysisStackTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(end_to_end)
{
    char const* text = R"(
        contract X {}
        contract Y {
            X x;
            constructor() public { x = new X(); }
        }
        contract Test {
            X x;
            Y y;
            constructor() public {
                x = new X();
                y = new Y();
            }
            function f(address _i) public { _i; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "Test");

    vector<ContractDefinition const*> model({ ctrt, ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    BOOST_CHECK_NE(stack->allocations().get(), nullptr);
    BOOST_CHECK_EQUAL(stack->model_cost(), 8);
    if (stack->allocations())
    {
        BOOST_CHECK_EQUAL(stack->allocations()->cost_of(ctrt), 4);
    }

    BOOST_CHECK_NE(stack->model().get(), nullptr);
    if (stack->model())
    {
        BOOST_CHECK_EQUAL(stack->model()->bundle().size(), 2);
    }

    BOOST_CHECK_NE(stack->calls().get(), nullptr);
    if (stack->calls())
    {
        BOOST_CHECK_EQUAL(stack->calls()->executed_code().size(), 3);
    }

    BOOST_CHECK_NE(stack->addresses().get(), nullptr);
    if (stack->addresses())
    {
        BOOST_CHECK_EQUAL(stack->addresses()->size(), 11);
    }

    BOOST_CHECK_NE(stack->types().get(), nullptr);
    BOOST_CHECK_NE(stack->environment().get(), nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}

