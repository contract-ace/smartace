/**
 * Tests for libsolidity/modelcheck/analysis/Inheritance.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/Inheritance.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>

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
    Analysis_InheritanceTests, ::dev::solidity::test::AnalysisFramework
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

BOOST_AUTO_TEST_CASE(modifier_order)
{
    char const* text = R"(
        contract A {
            modifier m1() { _; }
            modifier m2() { _; }
            modifier m3() { _; }
        }
        contract B is A {
            modifier m4() { _; }
            modifier m5() { _; }
        }
        contract C is B {
            modifier m2() { _; }
            modifier m4() { _; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_c = *retrieveContractByName(unit, "C");
    FlatContract flat(ctrt_c);

    BOOST_CHECK_EQUAL(flat.modifiers().size(), 5);
    if (flat.modifiers().size() == 5)
    {
        for (auto mod : flat.modifiers())
        {
            auto scope = dynamic_cast<ContractDefinition const*>(mod->scope());
            if (mod->name() == "m1")
            {
                BOOST_CHECK_EQUAL(scope->name(), "A");
            }
            else if (mod->name() == "m2")
            { 
                BOOST_CHECK_EQUAL(scope->name(), "C");
            }
            else if (mod->name() == "m3")
            {
                BOOST_CHECK_EQUAL(scope->name(), "A");
            }
            else if (mod->name() == "m4")
            {
                BOOST_CHECK_EQUAL(scope->name(), "C");
            }
            else if (mod->name() == "m5")
            {
                BOOST_CHECK_EQUAL(scope->name(), "B");
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(enums)
{
    char const* text = R"(
        contract A {
            enum EnumA { EnumA1, EnumA2 }
        }
        contract B is A {
            enum EnumB { EnumB1, EnumB2 }
        }
        contract C is B {
            enum EnumC { EnumC1, EnumC2 }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt_c = *retrieveContractByName(unit, "C");
    FlatContract flat(ctrt_c);

    BOOST_CHECK_EQUAL(flat.enums().size(), 3);
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

BOOST_AUTO_TEST_CASE(child_entries_are_added)
{
    char const* text = R"(
		contract A {}
        contract B {
            A a1;
            A a2;
            constructor() public {
                a1 = new A();
                a2 = new A();
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");

    vector<ContractDefinition const*> model({ ctrt_b });
    AllocationGraph graph(model);
    FlatModel flat_model(model, graph);

    auto children = flat_model.children_of(*flat_model.get(*ctrt_b));
    BOOST_CHECK_EQUAL(children.size(), 2);

    bool found_child_1 = false;
    bool found_child_2 = false;
    for (auto r : children)
    {
        if (r.child == flat_model.get(*ctrt_a))
        {
            if (r.var == "a1") found_child_1 = true;
            if (r.var == "a2") found_child_2 = true;
        }
    }
    BOOST_CHECK(found_child_1);
    BOOST_CHECK(found_child_2);
}

BOOST_AUTO_TEST_CASE(model_works)
{
    char const* text = R"(
		contract A {}
        contract B {}
        contract C {}
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");
    auto ctrt_c = retrieveContractByName(unit, "C");

    vector<ContractDefinition const*> model({
        ctrt_a, ctrt_a, ctrt_b, ctrt_b, ctrt_c, ctrt_a
    });
    AllocationGraph graph(model);

    FlatModel flat_model(model, graph);
    BOOST_CHECK_EQUAL(flat_model.view().size(), 3);
    BOOST_CHECK_EQUAL(flat_model.bundle().size(), 6);

    map<string, int> count;
    for (auto c : flat_model.bundle()) count[c->name()] += 1;
    BOOST_CHECK_EQUAL(count["A"], 3);
    BOOST_CHECK_EQUAL(count["B"], 2);
    BOOST_CHECK_EQUAL(count["C"], 1);
}

BOOST_AUTO_TEST_CASE(structs_work)
{
    char const* text = R"(
		contract A {
            struct B { int a; }
            struct C { int a; }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");

    vector<ContractDefinition const*> model({ ctrt_a });
    AllocationGraph graph(model);

    FlatModel flat_model(model, graph);
    BOOST_CHECK_EQUAL(flat_model.view().size(), 1);
    
    auto flat_a = flat_model.get(*ctrt_a);
    BOOST_CHECK_EQUAL(flat_a->structures().size(), 2);
    if (flat_a->structures().size() == 2)
    {
        BOOST_CHECK_EQUAL(flat_a->structures().front()->name(), "B");
        BOOST_CHECK_EQUAL(flat_a->structures().back()->name(), "C");
    }
}

BOOST_AUTO_TEST_CASE(mappings_work)
{
    char const* text = R"(
		contract A {
            mapping(address => mapping(address => uint)) a;
            mapping(address => uint) b;
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");

    vector<ContractDefinition const*> model({ ctrt_a });
    AllocationGraph graph(model);

    FlatModel flat_model(model, graph);
    BOOST_CHECK_EQUAL(flat_model.view().size(), 1);
    
    auto flat_a = flat_model.get(*ctrt_a);
    BOOST_CHECK_EQUAL(flat_a->mappings().size(), 2);
}

BOOST_AUTO_TEST_CASE(payable)
{
    char const* text = R"(
		contract A {
            function() external {}
        }
        contract B {}
        contract C {
            function() external payable {}
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_b = retrieveContractByName(unit, "B");
    auto ctrt_c = retrieveContractByName(unit, "C");

    vector<ContractDefinition const*> model({ ctrt_a, ctrt_b, ctrt_c });
    AllocationGraph graph(model);

    FlatModel flat_model(model, graph);
    BOOST_CHECK_EQUAL(flat_model.view().size(), 3);
    
    auto flat_a = flat_model.get(*ctrt_a);
    BOOST_CHECK(!flat_a->is_payable());
    
    auto flat_b = flat_model.get(*ctrt_b);
    BOOST_CHECK(!flat_b->is_payable());
    
    auto flat_c = flat_model.get(*ctrt_c);
    BOOST_CHECK(flat_c->is_payable());
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
