/**
 * Specific tests for libsolidity/modelcheck/model/ADT.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/model/ADT.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

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

// -------------------------------------------------------------------------- //

BOOST_FIXTURE_TEST_SUITE(
    Model_ADTTests, ::dev::solidity::test::AnalysisFramework
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
            mapping(address => mapping(address => mapping(address => int))) map3;
            struct B { mapping(address => mapping(address => int)) map1; }
            mapping(address => A) map4;
            struct C { mapping(address => int) map2; }
            B b;
            C c;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    ostringstream actual, expect;
    ADTConverter(stack, false, 1, true).print(actual);
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
            mapping(address => mapping(address => int)) map;
        }
    )";

    auto const &ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    ostringstream actual_k_1;
    ostringstream actual_k_2;

    ADTConverter(stack, false, 1, false).print(actual_k_1);
    ADTConverter(stack, false, 2, false).print(actual_k_2);

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
    auto ctrt_a = retrieveContractByName(unit, "A");
    auto ctrt_c = retrieveContractByName(unit, "C");

    vector<ContractDefinition const*> model({ ctrt_a, ctrt_c });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    ostringstream actual, expect;
    ADTConverter(stack, false, 1, false).print(actual);
    expect << "struct A"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "sol_int256_t user_a;"
           << "sol_int256_t user_b;"
           << "sol_int256_t user_c;"
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
    auto ctrt = retrieveContractByName(unit, "Test");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    ostringstream actual, expect;
    ADTConverter(stack, false, 1, false).print(actual);
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

BOOST_AUTO_TEST_CASE(constants)
{
    char const* text = R"(
        contract A {
            int a; int constant b = 5;
        }
    )";

    auto const &unit = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(unit, "A");

    vector<ContractDefinition const*> model({ ctrt_a });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    ostringstream actual, expect;
    ADTConverter(stack, false, 1, false).print(actual);
    expect << "struct A"
           << "{"
           << "sol_address_t model_address;"
           << "sol_uint256_t model_balance;"
           << "sol_int256_t user_a;"
           << "};";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
