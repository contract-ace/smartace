/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/translation/ADT.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>
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

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    ADTConverter(ast, converter, 1, true).print(actual);
    expect << "struct A_B_map1_submap2;";
    expect << "struct A_B_map1_submap1;";
    expect << "struct A_B;";
    expect << "struct A_C_map2_submap1;";
    expect << "struct A_C;";
    expect << "struct A_map3_submap3;";
    expect << "struct A_map3_submap2;";
    expect << "struct A_map3_submap1;";
    expect << "struct A_map4_submap1;";
}

// Tests that maps use the correct array lengths. This test is fairly naive and
// prone to false negatives. There are no false positives, provided C-array
// syntax is used.
BOOST_AUTO_TEST_CASE(map_internal_repr)
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

    TypeConverter converter;
    converter.record(ast);

    for (int i = 1; i <= 16; ++i)
    {
        ostringstream actual;
        ADTConverter(ast, converter, i, false).print(actual);

        for (int j = 0; j <= 16; ++j)
        {
            ostringstream target;
            target << "curr" << j;
            if (j < i)
            {
                BOOST_CHECK(actual.str().find(target.str()) != string::npos);
            }
            else
            {
                BOOST_CHECK(actual.str().find(target.str()) == string::npos);
            }
        }
    }
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

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expect;
    ADTConverter(ctrt, converter, 1, false).print(actual);
    expect << "struct Escrow"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "sol_uint256_t user_a;"
           << "sol_uint256_t user_b;"
           << "sol_uint256_t user_c;"
           << "sol_uint256_t user_d;"
           << "sol_uint256_t user_e;"
           << "sol_uint256_t user_f;"
           << "sol_uint256_t user_g;"
           << "}";
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
