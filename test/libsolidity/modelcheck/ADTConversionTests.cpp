/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/ADTConverter.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>
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
            struct B { mapping(int => mapping(ing => int)) map1; }
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
    ADTConverter(ast, converter, true).print(actual);
    expect << "struct A_B_map1_submap2;" << endl;
    expect << "struct A_B_map1_submap1;" << endl;
    expect << "struct A_B;" << endl;
    expect << "struct A_C_map2_submap1;" << endl;
    expect << "struct A_C;" << endl;
    expect << "struct A_map3_submap3;" << endl;
    expect << "struct A_map3_submap2;" << endl;
    expect << "struct A_map3_submap1;" << endl;
    expect << "struct A_map4_submap1;" << endl;
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
