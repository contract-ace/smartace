/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/model/ADT.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>

#include <map>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(
    ADTConversionTests,
    ::dev::solidity::test::AnalysisFramework
)

// In the c-model, a contract is converted into a set of structs, which
// correspond to maps, structures, and the contract itself. All maps must be
// declared in bottom-up order, before the structure or contract they are
// contained in. All structures must be declared before their contract. If a
// structure is used in a map, it must be declared before that map. This test
// ensures a contract's ADT order is,
// 1. All structures, in declaration order
//      1a. Each map within that structure, in bottom-up order
//      1b. The structure, itself
// 2. All maps within the contract
// 3. The contract, itself
BOOST_AUTO_TEST_CASE(contract_internal_dependency_order)
{
    char const* text = R"(
        contract A {
            mapping(int => mapping(int => mapping(int => int))) map3;
            struct B { mapping(int => mapping(int => int)) map1; }
            mapping(int => A) map4;
            struct C { mapping(int => int) map2; }
            B b;
            C c;
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeAnalyzer converter;
    converter.record(ast);

    NewCallGraph callgraph;
    callgraph.record(ast);
    callgraph.finalize();

    ostringstream actual, expect;
    ADTConverter(ast, callgraph, converter, false, 1, true).print(actual);
    expect << "struct Map_1;";
    expect << "struct A_Struct_B;";
    expect << "struct Map_2;";
    expect << "struct A_Struct_C;";
    expect << "struct Map_3;";
    expect << "struct Map_4;";
    expect << "struct A;";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Tests that maps use the correct array lengths. This test is fairly naive and
// prone to false negatives. There are no false positives, provided C-array
// syntax is used.
BOOST_AUTO_TEST_CASE(map_internal_repr)
{
    char const* text = R"(
        contract A {
            mapping(int => mapping(int => int)) map;
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeAnalyzer converter;
    converter.record(ast);

    NewCallGraph callgraph;
    callgraph.record(ast);
    callgraph.finalize();

    ostringstream actual_k_1;
    ostringstream actual_k_2;

    ADTConverter(ast, callgraph, converter, false, 1, false).print(actual_k_1);
    ADTConverter(ast, callgraph, converter, false, 2, false).print(actual_k_2);

    BOOST_CHECK(actual_k_1.str().find("data_0_0") != string::npos);

    BOOST_CHECK(actual_k_2.str().find("data_0_0") != string::npos);
    BOOST_CHECK(actual_k_2.str().find("data_0_1") != string::npos);
    BOOST_CHECK(actual_k_2.str().find("data_1_0") != string::npos);
    BOOST_CHECK(actual_k_2.str().find("data_1_1") != string::npos);
}

BOOST_AUTO_TEST_CASE(member_inheritance)
{
    char const* text = R"(
        contract A {
            int a; int b; int c;
        }
        contract B is A {
            int c; int d; int e;
        }
        contract C is B {
            int e; int f; int g;
        }
    )";

    auto const &unit = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(unit, "C");

    TypeAnalyzer converter;
    converter.record(unit);

    NewCallGraph callgraph;
    callgraph.record(unit);
    callgraph.finalize();

    ostringstream actual, expect;
    ADTConverter(ctrt, callgraph, converter, false, 1, false).print(actual);
    expect << "struct A"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "sol_int256_t user_a;"
           << "sol_int256_t user_b;"
           << "sol_int256_t user_c;"
           << "};";
    expect << "struct B"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "sol_int256_t user_c;"
           << "sol_int256_t user_d;"
           << "sol_int256_t user_e;"
           << "sol_int256_t user_a;"
           << "sol_int256_t user_b;"
           << "};";
    expect << "struct C"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "sol_int256_t user_e;"
           << "sol_int256_t user_f;"
           << "sol_int256_t user_g;"
           << "sol_int256_t user_c;"
           << "sol_int256_t user_d;"
           << "sol_int256_t user_a;"
           << "sol_int256_t user_b;"
           << "};";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

 BOOST_AUTO_TEST_CASE(specialize_contracts)
 {
    char const* text = R"(
        contract X {}

        contract Y is X {}

        contract Test {
            X x;
            constructor() public { x = new Y(); }
        }
    )";

    auto const &unit = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(unit, "Test");

    TypeAnalyzer converter;
    converter.record(unit);

    NewCallGraph callgraph;
    callgraph.record(unit);
    callgraph.finalize();

    ostringstream actual, expect;
    ADTConverter(ctrt, callgraph, converter, false, 1, false).print(actual);
    expect << "struct X"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "};";
    expect << "struct Y"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "};";
    expect << "struct Test"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "struct Y user_x;"
           << "};";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
 }

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
