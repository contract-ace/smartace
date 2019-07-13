/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/FunctionConverter.h>

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
    FunctionConversionTests,
    ::dev::solidity::test::AnalysisFramework
)

// Ensures that each contract generates a function `Init_<contract>()`. This
// method should set all simple members to 0, while setting all complex members
// with their default constructors.
BOOST_AUTO_TEST_CASE(default_constructors)
{
    char const* text = R"(
        contract A {
            struct B {
                uint a;
            }
            uint a;
            uint b = 10;
            B c;
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    FunctionConverter(ast, converter, false).print(actual);
    // -- Init_A
    expect << "struct A Init_A()";
    expect << "{";
    expect << "struct A tmp;";
    expect << "tmp.d_a=0;";
    expect << "tmp.d_b=10;";
    expect << "tmp.d_c=Init_A_B();";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_B
    expect << "struct A_B Init_A_B(unsigned int a=0)";
    expect << "{";
    expect << "struct A_B tmp;";
    expect << "tmp.d_a=a;";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_B
    expect << "struct A_B ND_A_B()";
    expect << "{";
    expect << "struct A_B tmp;";
    expect << "tmp.d_a=ND_Init_Val();";
    expect << "return tmp;";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Ensures that custom constructors generate the correct code. It should be a
// void function, named `Ctor_<contract>(<v1>, ..., <bn>)`.
BOOST_AUTO_TEST_CASE(custom_constructors)
{
    char const* text = R"(
        contract A {
            constructor(uint _a) public {
                a = _a;
            }
            uint a;
            uint b;
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    FunctionConverter(ast, converter, false).print(actual);
    // -- Init_A
    expect << "struct A Init_A(struct A *self,struct CallState *state"
           << ",unsigned int _a)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "tmp.d_a=0;";
    expect << "tmp.d_b=0;";
    expect << "Ctor_A(&tmp,state,_a);";
    expect << "return tmp;";
    expect << "}";
    // -- Ctor_A
    expect << "void Ctor_A(struct A *self,struct CallState *state"
           << ",unsigned int _a)";
    expect << "{";
    expect << "(self->d_a)=(_a);";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
