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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Init_A
    expect << "struct A Init_A()";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).d_a)=(Init_uint256_t(0));";
    expect << "((tmp).d_b)=(Init_uint256_t(10));";
    expect << "((tmp).d_c)=(Init_0_A_StructB());";
    expect << "return tmp;";
    expect << "}";
    // -- Init_0_A_StructB
    expect << "struct A_StructB Init_0_A_StructB()";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).d_a)=(Init_uint256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_StructB
    expect << "struct A_StructB Init_A_StructB(uint256_t a)";
    expect << "{";
    expect << "struct A_StructB tmp=Init_0_A_StructB();";
    expect << "((tmp).d_a)=(a);";
    expect << "return tmp;";
    expect << "}";
    // -- A_StructB
    expect << "struct A_StructB ND_A_StructB()";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).d_a)=(Init_uint256_t(ND_Init_Val()));";
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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Init_A
    expect << "struct A Init_A(struct A*self,struct CallState*state"
           << ",uint256_t _a)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).d_a)=(Init_uint256_t(0));";
    expect << "((tmp).d_b)=(Init_uint256_t(0));";
    expect << "Ctor_A(&(tmp),state,_a);";
    expect << "return tmp;";
    expect << "}";
    // -- Ctor_A
    expect << "void Ctor_A(struct A*self,struct CallState*state,uint256_t _a)";
    expect << "{";
    expect << "((self->d_a).v)=((_a).v);";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Ensure a mix of default and manual struct fields work.
BOOST_AUTO_TEST_CASE(struct_initialization)
{
    char const* text = R"(
        contract A {
            struct B {
                int i1;
            }
            struct C {
                int i1;
                B b1;
                int i2;
                uint ui1;
                B b2;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Init_A
    expect << "struct A Init_A()";
    expect << "{";
    expect << "struct A tmp;";
    expect << "return tmp;";
    expect << "}";
    // -- Init_0_A_StructB
    expect << "struct A_StructB Init_0_A_StructB()";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).d_i1)=(Init_int256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_StructB
    expect << "struct A_StructB Init_A_StructB(int256_t i1)";
    expect << "{";
    expect << "struct A_StructB tmp=Init_0_A_StructB();";
    expect << "((tmp).d_i1)=(i1);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_StructB
    expect << "struct A_StructB ND_A_StructB()";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).d_i1)=(Init_int256_t(ND_Init_Val()));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_0_A_StructC
    expect << "struct A_StructC Init_0_A_StructC()";
    expect << "{";
    expect << "struct A_StructC tmp;";
    expect << "((tmp).d_i1)=(Init_int256_t(0));";
    expect << "((tmp).d_b1)=(Init_0_A_StructB());";
    expect << "((tmp).d_i2)=(Init_int256_t(0));";
    expect << "((tmp).d_ui1)=(Init_uint256_t(0));";
    expect << "((tmp).d_b2)=(Init_0_A_StructB());";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_StructC
    expect << "struct A_StructC Init_A_StructC(int256_t i1,int256_t i2"
           << ",uint256_t ui1)";
    expect << "{";
    expect << "struct A_StructC tmp=Init_0_A_StructC();";
    expect << "((tmp).d_i1)=(i1);";
    expect << "((tmp).d_i2)=(i2);";
    expect << "((tmp).d_ui1)=(ui1);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_StructB
    expect << "struct A_StructC ND_A_StructC()";
    expect << "{";
    expect << "struct A_StructC tmp;";
    expect << "((tmp).d_i1)=(Init_int256_t(ND_Init_Val()));";
    expect << "((tmp).d_b1)=(ND_A_StructB());";
    expect << "((tmp).d_i2)=(Init_int256_t(ND_Init_Val()));";
    expect << "((tmp).d_ui1)=(Init_uint256_t(ND_Init_Val()));";
    expect << "((tmp).d_b2)=(ND_A_StructB());";
    expect << "return tmp;";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Ensures the function converter can be set to hide implementation details.
BOOST_AUTO_TEST_CASE(can_hide_internals)
{
    char const* text = R"(
        contract A {
            struct B {
                int i;
            }
            mapping(int => int) m;
            function f() public pure {}
            function g() private pure {}
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream ext_actual, ext_expect;
    FunctionConverter(
        ast, converter, FunctionConverter::View::EXT, true
    ).print(ext_actual);
    ext_expect << "struct A Init_A();";
    ext_expect << "void Method_A_Funcf();";

    ostringstream int_actual, int_expect;
    FunctionConverter(
        ast, converter, FunctionConverter::View::INT, true
    ).print(int_actual);
    int_expect << "struct A_StructB Init_0_A_StructB();";
    int_expect << "struct A_StructB Init_A_StructB(int256_t i);";
    int_expect << "struct A_StructB ND_A_StructB();";
    int_expect << "struct A_Mapm_submap1 Init_0_A_Mapm_submap1();";
    int_expect << "struct A_Mapm_submap1 ND_A_Mapm_submap1();";
    int_expect << "int256_t Read_A_Mapm_submap1(struct A_Mapm_submap1*a,"
               << "int256_t idx);";
    int_expect << "void Write_A_Mapm_submap1(struct A_Mapm_submap1*a,"
               << "int256_t idx,int256_t d);";
    int_expect << "int256_t*Ref_A_Mapm_submap1(struct A_Mapm_submap1*a"
               << ",int256_t idx);";
    int_expect << "void Method_A_Funcg();";

    BOOST_CHECK_EQUAL(ext_actual.str(), ext_expect.str());
    BOOST_CHECK_EQUAL(int_actual.str(), int_expect.str());
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
