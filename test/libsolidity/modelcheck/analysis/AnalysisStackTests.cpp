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
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Library.h>
#include <libsolidity/modelcheck/analysis/TightBundle.h>
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

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);

    BOOST_CHECK_NE(stack->allocations().get(), nullptr);
    if (stack->allocations())
    {
        BOOST_CHECK_EQUAL(stack->allocations()->cost_of(ctrt), 4);
    }

    BOOST_CHECK_NE(stack->model().get(), nullptr);
    shared_ptr<FlatContract> flat;
    if (stack->model())
    {
        BOOST_CHECK_EQUAL(stack->model()->bundle().size(), 2);
        flat = stack->model()->get(*ctrt);
    }

    BOOST_CHECK_NE(stack->tight_bundle().get(), nullptr);
    if (stack->tight_bundle())
    {
        BOOST_CHECK_EQUAL(stack->tight_bundle()->size(), 8);
    }

    BOOST_CHECK_NE(stack->contracts().get(), nullptr);
    if (stack->contracts() && flat)
    {
        auto stmt = ctrt->definedFunctions()[0]->body().statements()[0];
        auto expr_stmt = dynamic_cast<ExpressionStatement const*>(stmt.get());
        auto assign = dynamic_cast<Assignment const*>(&expr_stmt->expression());
        auto id = (&assign->leftHandSide());
        BOOST_CHECK_EQUAL(stack->contracts()->resolve(*id, flat)->name(), "X");
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

BOOST_AUTO_TEST_CASE(libraries)
{
    char const* text = R"(
        library Lib1 {
            function f() public pure {}
        }
        library Lib2 {
            function f() public pure {}
        }
        library Lib3 {
            function f() public pure {}
        }
        contract A {
            function f() public pure {
                Lib1.f();
                Lib2.f();
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);

    auto libraries = stack->libraries()->view();
    BOOST_CHECK_EQUAL(libraries.size(), 2);
    
    set<string> names;
    for (auto lib : libraries) names.insert(lib->name());
    BOOST_CHECK(names.find("Lib1") != names.end());
    BOOST_CHECK(names.find("Lib2") != names.end());
}

BOOST_AUTO_TEST_CASE(params)
{
    char const* text = R"(
        contract X {
            function f() public {}
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });

    AnalysisSettings nparam_set;
    nparam_set.persistent_user_count = 0;
    nparam_set.use_concrete_users = false;
    nparam_set.use_global_contracts = false;
    nparam_set.escalate_reqs = false;
    auto stack_nparam = make_shared<AnalysisStack>(model, full, nparam_set);

    AnalysisSettings wparam_set;
    wparam_set.persistent_user_count = 5;
    wparam_set.use_concrete_users = true;
    wparam_set.use_global_contracts = false;
    wparam_set.escalate_reqs = true;
    auto stack_wparam = make_shared<AnalysisStack>(model, full, wparam_set);

    BOOST_CHECK_EQUAL(stack_nparam->addresses()->size(), 3);
    BOOST_CHECK_EQUAL(stack_nparam->addresses()->max_interference(), 1);
    BOOST_CHECK(!stack_nparam->environment()->escalate_requires());

    BOOST_CHECK_EQUAL(stack_wparam->addresses()->size(), 7);
    BOOST_CHECK_EQUAL(stack_wparam->addresses()->max_interference(), 0);
    BOOST_CHECK(stack_wparam->environment()->escalate_requires());
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}

