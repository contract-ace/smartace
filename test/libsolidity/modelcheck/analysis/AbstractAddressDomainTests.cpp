/**
 * Tests for libsolidity/modelcheck/analysis/AbstractAddressDomain.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>

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
    Analysis_AbstractAddressDomainTests,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(address_to_int)
{
    char const* text = R"(
        contract X {
            int a;
            function f(address i) public {
                a += 1;
            }
            function g(address i) public pure {
                uint160(i);
            }
            function h(address i) public {
                a -= 1;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    auto violations = ptg.violations();
    BOOST_CHECK_EQUAL(violations.size(), 1);
    if (violations.size() == 1)
    {
        BOOST_CHECK_EQUAL(violations.front().context()->name(), "g");
        BOOST_CHECK(
            violations.front().type() == AddressViolation::Type::Cast
        );
    }

    LiteralExtractor lext(*flat_model, call_graph);
    BOOST_CHECK_EQUAL(lext.violations().size(), 1);
}

BOOST_AUTO_TEST_CASE(int_to_address_valid)
{
    char const* text = R"(
        contract X {
            function f() public {
                address(1);
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    BOOST_CHECK(ptg.violations().empty());

    LiteralExtractor lext(*flat_model, call_graph);
    BOOST_CHECK(lext.violations().empty());
}

BOOST_AUTO_TEST_CASE(int_to_address_invalid)
{
    char const* text = R"(
        contract X {
            struct A { uint160 i; }
            A a;
            function f(uint160 i) public {
                address(i);
            }
            function g(uint160 i) public {
                address(i + 1);
            }
            function h() public view {
                address(a.i);
            }
            function p() public pure returns (uint160) {
                return 10;
            }
            function q() public pure {
                address(p());
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    auto violations = ptg.violations();
    BOOST_CHECK_EQUAL(violations.size(), 4);
    for (auto itr : violations)
    {
        BOOST_CHECK(itr.type() == AddressViolation::Type::Mutate);
    }

    LiteralExtractor lext(*flat_model, call_graph);
    BOOST_CHECK_EQUAL(lext.violations().size(), 4);
}

BOOST_AUTO_TEST_CASE(comparisons)
{
    char const* text = R"(
        contract X {
            function f(address i, address j) public {
                i == j;
                i != j;
            }
            function g(address i, address j) public pure {
                i < j;
                i > j;
                i <= j;
                i >= j;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    auto violations = ptg.violations();
    BOOST_CHECK_EQUAL(violations.size(), 4);
    for (auto itr : violations)
    {
        BOOST_CHECK_EQUAL(itr.context()->name(), "g");
        BOOST_CHECK(itr.type() == AddressViolation::Type::Compare);
    }

    LiteralExtractor lext(*flat_model, call_graph);
    BOOST_CHECK_EQUAL(lext.violations().size(), 4);
}

BOOST_AUTO_TEST_CASE(literals)
{
    char const* text = R"(
        contract X {
            function f() public pure {
                address(4);
                address(10);
                address i;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    auto literals = ptg.literals();
    BOOST_CHECK_EQUAL(literals.size(), 3);
    BOOST_CHECK_EQUAL(ptg.contract_count(), 5);
    BOOST_CHECK_EQUAL(ptg.implicit_count(), 13);
    BOOST_CHECK_EQUAL(ptg.interference_count(), 1);
    BOOST_CHECK_EQUAL(ptg.size(), 14);
    if (literals.size() == 3)
    {
        BOOST_CHECK(literals.find(dev::u256(0)) != literals.end());
        BOOST_CHECK(literals.find(dev::u256(4)) != literals.end());
        BOOST_CHECK(literals.find(dev::u256(10)) != literals.end());
    }
    BOOST_CHECK(ptg.violations().empty());

    LiteralExtractor lext(*flat_model, call_graph);
    BOOST_CHECK_EQUAL(lext.literals().size(), 3);
    BOOST_CHECK(lext.violations().empty());

    RoleExtractor rext(converter.map_db(), *flat_model->get(ctrt));
    BOOST_CHECK_EQUAL(rext.count(), 0);
    BOOST_CHECK(rext.violations().empty());

    ClientExtractor cext(*flat_model);
    BOOST_CHECK_EQUAL(cext.count(), 1);
}

BOOST_AUTO_TEST_CASE(mixed_summary)
{
    char const* text = R"(
        contract X {
            function f() public pure {
                address i;
            }
            function g(uint160 i) public pure {
                address(i);
            }
        }
        contract Y {
            function f() public pure {
                address i = address(56);
            }
            function g(uint160 i) public pure {
                address(i);
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt1 = *retrieveContractByName(unit, "X");
    auto const& ctrt2 = *retrieveContractByName(unit, "Y");

    StructureStore store_1;
    vector<ContractDefinition const*> model_1({ &ctrt1 });
    auto alloc_graph_1 = make_shared<AllocationGraph>(model_1);
    auto flat_model_1 = make_shared<FlatModel>(model_1, *alloc_graph_1, store_1);
    auto r_1 = make_shared<ContractExpressionAnalyzer>(flat_model_1, alloc_graph_1);
    CallGraph call_graph_1(r_1, flat_model_1);
    TypeAnalyzer converter_1({ &unit }, call_graph_1);
    PTGBuilder ptg_1(converter_1.map_db(), *flat_model_1, call_graph_1, false, 5, 5);

    BOOST_CHECK_EQUAL(ptg_1.violations().size(), 1);
    BOOST_CHECK_EQUAL(ptg_1.literals().size(), 1);

    StructureStore store_2;
    vector<ContractDefinition const*> model_2({ &ctrt1, &ctrt2 });
    auto alloc_graph_2 = make_shared<AllocationGraph>(model_2);
    auto flat_model_2 = make_shared<FlatModel>(model_2, *alloc_graph_2, store_2);
    auto r_2 = make_shared<ContractExpressionAnalyzer>(flat_model_2, alloc_graph_2);
    CallGraph call_graph_2(r_2, flat_model_2);
    TypeAnalyzer converter_2({ &unit }, call_graph_2);
    PTGBuilder ptg_2(converter_2.map_db(), *flat_model_2, call_graph_2, false, 5, 5);

    BOOST_CHECK_EQUAL(ptg_2.violations().size(), 2);
    BOOST_CHECK_EQUAL(ptg_2.literals().size(), 2);
}

BOOST_AUTO_TEST_CASE(basic_interference_count)
{
    char const* text = R"(
        contract X {
            function f() public pure { }
            function f(address i, address j) public pure { }
            function f(address j) public pure { }
        }
        contract Y {
            address k;
            address l;
            function f() public pure { }
            function f(address i, address j) public pure { }
            function f(address j) public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt1 = *retrieveContractByName(unit, "X");
    auto const& ctrt2 = *retrieveContractByName(unit, "Y");

    StructureStore store_1;
    vector<ContractDefinition const*> model_1({ &ctrt1 });
    auto alloc_graph_1 = make_shared<AllocationGraph>(model_1);
    auto flat_model_1 = make_shared<FlatModel>(model_1, *alloc_graph_1, store_1);
    auto r_1 = make_shared<ContractExpressionAnalyzer>(flat_model_1, alloc_graph_1);
    CallGraph call_graph_1(r_1, flat_model_1);
    TypeAnalyzer converter_1({ &unit }, call_graph_1);
    PTGBuilder ptg_1(converter_1.map_db(), *flat_model_1, call_graph_1, false, 5, 5);

    BOOST_CHECK_EQUAL(ptg_1.interference_count(), 3);

    StructureStore store_2;
    vector<ContractDefinition const*> model_2({ &ctrt1, &ctrt2 });
    auto alloc_graph_2 = make_shared<AllocationGraph>(model_2);
    auto flat_model_2 = make_shared<FlatModel>(model_2, *alloc_graph_2, store_2);
    auto r_2 = make_shared<ContractExpressionAnalyzer>(flat_model_2, alloc_graph_2);
    CallGraph call_graph_2(r_2, flat_model_2);
    TypeAnalyzer converter_2({ &unit }, call_graph_2);
    PTGBuilder ptg_2(converter_2.map_db(), *flat_model_2, call_graph_2, false, 5, 5);

    BOOST_CHECK_EQUAL(ptg_2.interference_count(), 5);
}

BOOST_AUTO_TEST_CASE(compound_interference_count)
{
    char const* text = R"(
        contract X {
            struct A {
                address x;
                address y;
            }
            struct B {
                A a1;
                A a2;
                address z;
            }
            B b;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    BOOST_CHECK_EQUAL(ptg.literals().size(), 1);
    BOOST_CHECK_EQUAL(ptg.contract_count(), 5);
    BOOST_CHECK_EQUAL(ptg.implicit_count(), 11);
    BOOST_CHECK_EQUAL(ptg.interference_count(), 6);
    BOOST_CHECK_EQUAL(ptg.size(), 17);
    BOOST_CHECK(ptg.violations().empty());
}

BOOST_AUTO_TEST_CASE(basic_map_of_addrs_count)
{
    char const* text = R"(
        contract X {
            mapping(address => mapping(address => address)) m;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    auto violations = ptg.violations();
    BOOST_CHECK_EQUAL(violations.size(), 1);
    if (violations.size() == 1)
    {
        BOOST_CHECK(
            violations.front().type() == AddressViolation::Type::ValueType
        );
    }

    RoleExtractor rext(converter.map_db(), *flat_model->get(ctrt));
    BOOST_CHECK_EQUAL(rext.violations().size(), 1);
}

BOOST_AUTO_TEST_CASE(basic_map_of_structs_count)
{
    char const* text = R"(
        contract X {
            struct A {
                address x;
                address y;
            }
            mapping(address => mapping(address => A)) m;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    auto violations = ptg.violations();
    BOOST_CHECK_EQUAL(violations.size(), 1);
    if (violations.size() == 1)
    {
        BOOST_CHECK(
            violations.front().type() == AddressViolation::Type::ValueType
        );
    }

    RoleExtractor rext(converter.map_db(), *flat_model->get(ctrt));
    BOOST_CHECK_EQUAL(rext.violations().size(), 1);
}




BOOST_AUTO_TEST_CASE(inherited_addr_count)
{
    char const* text = R"(
        contract Y {
            address y;
        }
        contract X is Y {
            address x;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 5, 5);

    BOOST_CHECK_EQUAL(ptg.literals().size(), 1);
    BOOST_CHECK_EQUAL(ptg.contract_count(), 5);
    BOOST_CHECK_EQUAL(ptg.implicit_count(), 11);
    BOOST_CHECK_EQUAL(ptg.interference_count(), 3);
    BOOST_CHECK_EQUAL(ptg.size(), 14);
    BOOST_CHECK(ptg.violations().empty());
}

BOOST_AUTO_TEST_CASE(concrete_test)
{
    char const* text = R"(
        contract X {
            address x;
            function f(address y) public {
                address(1);
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, true, 2, 1);

    BOOST_CHECK_EQUAL(ptg.literals().size(), 2);
    BOOST_CHECK_EQUAL(ptg.contract_count(), 2);
    BOOST_CHECK_EQUAL(ptg.implicit_count(), 5);
    BOOST_CHECK_EQUAL(ptg.interference_count(), 0);
    BOOST_CHECK_EQUAL(ptg.size(), 5);
    BOOST_CHECK(ptg.violations().empty());
}

BOOST_AUTO_TEST_CASE(noop_address_cast)
{
    char const* text = R"(
        contract X {
            function f(address i) public {
                address(i);
                address(address(5));
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 2, 1);

    BOOST_CHECK_EQUAL(ptg.literals().size(), 2);
    BOOST_CHECK_EQUAL(ptg.contract_count(), 2);
    BOOST_CHECK_EQUAL(ptg.implicit_count(), 5);
    BOOST_CHECK_EQUAL(ptg.interference_count(), 2);
    BOOST_CHECK_EQUAL(ptg.size(), 7);
    BOOST_CHECK(ptg.violations().empty());
}

BOOST_AUTO_TEST_CASE(role_summary)
{
    char const* text = R"(
        contract X {
            struct A {
                address x;
                address y;
            }
            struct B {
                A a;
                address z;
            }
            A a;
            B b;
            address x;
            function f() public pure { }
        }
        contract Y {
            address k;
            address l;
            function f() public pure { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt1 = *retrieveContractByName(unit, "X");
    auto const& ctrt2 = *retrieveContractByName(unit, "Y");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt1, &ctrt2 });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 2, 1);

    BOOST_CHECK_EQUAL(ptg.summarize(flat_model->get(ctrt1)).size(), 3);
    BOOST_CHECK_EQUAL(ptg.summarize(flat_model->get(ctrt2)).size(), 2);
}

BOOST_AUTO_TEST_CASE(rv_contract_to_addr)
{
    char const* text = R"(
        contract A {}
        contract X {
            A a;
            constructor() public { a = new A(); }
            function get() public view returns (A) { return a; }
            function test() public view { address(get()); }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 2, 1);

    BOOST_CHECK(ptg.violations().empty());
}

BOOST_AUTO_TEST_CASE(fallback)
{
    char const* text = R"(
        contract X {
            function () external payable {
                address(5);
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(unit, "X");

    StructureStore store;
    vector<ContractDefinition const*> model({ &ctrt });
    auto alloc_graph = make_shared<AllocationGraph>(model);
    auto flat_model = make_shared<FlatModel>(model, *alloc_graph, store);
    auto r = make_shared<ContractExpressionAnalyzer>(flat_model, alloc_graph);
    CallGraph call_graph(r, flat_model);
    TypeAnalyzer converter({ &unit }, call_graph);
    PTGBuilder ptg(converter.map_db(), *flat_model, call_graph, false, 2, 1);

    BOOST_CHECK_EQUAL(ptg.literals().size(), 2);
    BOOST_CHECK_EQUAL(ptg.contract_count(), 2);
    BOOST_CHECK_EQUAL(ptg.implicit_count(), 5);
    BOOST_CHECK_EQUAL(ptg.interference_count(), 1);
    BOOST_CHECK_EQUAL(ptg.size(), 6);
    BOOST_CHECK(ptg.violations().empty());
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
