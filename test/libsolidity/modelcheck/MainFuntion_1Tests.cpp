/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/MainFunction_1.h>

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
    MainFunction_1Tests,
    ::dev::solidity::test::AnalysisFramework
)

// Ensures that each contract generates a function `Init_<contract>()`. This
// method should set all simple members to 0, while setting all complex members
// with their default constructors.
BOOST_AUTO_TEST_CASE(CallState_Struct)
{
    char const* text = R"(
      contract A {

      }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    MainFunction_1(ast, converter, false).print(actual);
    // -- Init_A
    expect << "struct CallState";
    expect << "{";
    expect << "int sender;";
    expect << "unsigned int value;";
    expect << "unsigned int blocknum;";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}


BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
