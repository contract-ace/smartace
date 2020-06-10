/**
 * @date 2019
 * Test suite targeting function manipulation utilities.
 */

#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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
    Inheritance,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(interface)
{
    char const* text = R"(
        contract A {
            function f() public pure {} // New
            function f(uint a) public pure {} // New
        }
        contract B is A {
            constructor() public {}
            function f() public pure {}
            function f(uint a, uint b) public pure {} // New
            function g() public pure {} // New
        }
        contract C is B {
            constructor() public {}
            function h() private pure {} // New
            function() external {}
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");
    const auto& ctrt_c = *retrieveContractByName(unit, "C");

    BOOST_CHECK_EQUAL(FlatContract(ctrt_a).interface().size(), 2);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_b).interface().size(), 4);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_c).interface().size(), 4);
}

BOOST_AUTO_TEST_CASE(variables)
{
    char const* text = R"(
		contract A {
            int a; // New.
            int b; // New.
            int c; // New.
		}
        contract B is A {
            int a;
            int d; // New.
            int e; // New.
        }
        contract C is B {
            int b;
            int e;
            int f; // New.
        }
        contract D {
            int a;
            int g; // New.
            int h; // New.
        }
        contract E is C, D {
            int b;
            int g;
            int i; // New.
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");
    const auto& ctrt_c = *retrieveContractByName(unit, "C");
    const auto& ctrt_d = *retrieveContractByName(unit, "D");
    const auto& ctrt_e = *retrieveContractByName(unit, "E");

    BOOST_CHECK_EQUAL(FlatContract(ctrt_a).state_variables().size(), 3);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_b).state_variables().size(), 5);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_c).state_variables().size(), 6);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_d).state_variables().size(), 3);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_e).state_variables().size(), 9);
}

BOOST_AUTO_TEST_CASE(constructors)
{
    char const* text = R"(
		contract A {}
        contract B is A { constructor() public {} }
        contract C is B { constructor() public {} }
        contract D { constructor() public {} }
        contract E is C, D {}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");
    const auto& ctrt_c = *retrieveContractByName(unit, "C");
    const auto& ctrt_e = *retrieveContractByName(unit, "E");

    BOOST_CHECK_EQUAL(FlatContract(ctrt_a).constructors().size(), 0);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_b).constructors().size(), 1);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_c).constructors().size(), 2);
    BOOST_CHECK_EQUAL(FlatContract(ctrt_e).constructors().size(), 3);
}

BOOST_AUTO_TEST_CASE(fallback)
{
    char const* text = R"(
		contract A {}
        contract B is A { function() external {} }
        contract C is B {}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_a = *retrieveContractByName(unit, "A");
    const auto& ctrt_b = *retrieveContractByName(unit, "B");
    const auto& ctrt_c = *retrieveContractByName(unit, "C");

    BOOST_CHECK(FlatContract(ctrt_a).fallback() == nullptr);
    BOOST_CHECK(FlatContract(ctrt_b).fallback() != nullptr);
    BOOST_CHECK(FlatContract(ctrt_c).fallback() != nullptr);
}

BOOST_AUTO_TEST_CASE(model)
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

    vector<ContractDefinition const*> model({ ctrt_d, ctrt_e });
    AllocationGraph graph(model);
    FlatModel flat_model(model, graph);

    BOOST_CHECK_EQUAL(flat_model.view().size(), 5);

    set<string> contracts;
    for (auto contract : flat_model.view())
    {
        BOOST_CHECK(contracts.insert(contract->name()).second);
    }
    BOOST_CHECK(contracts.find("A") != contracts.end());
    BOOST_CHECK(contracts.find("B") != contracts.end());
    BOOST_CHECK(contracts.find("C") != contracts.end());
    BOOST_CHECK(contracts.find("D") != contracts.end());
    BOOST_CHECK(contracts.find("E") != contracts.end());
}

BOOST_AUTO_TEST_CASE(super_classes_in_model)
{
    char const* text = R"(
		contract A {}
        contract B is A {}
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");

    vector<ContractDefinition const*> model({ ctrt_b });
    AllocationGraph graph(model);
    FlatModel flat_model(model, graph);

    BOOST_CHECK_EQUAL(flat_model.view().size(), 1);
    BOOST_CHECK_NE(flat_model.get(*ctrt_a), nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
