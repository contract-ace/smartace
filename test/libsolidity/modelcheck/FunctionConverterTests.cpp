/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/translation/Function.h>

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

// Regression test to ensure returns of wrapped types work.
BOOST_AUTO_TEST_CASE(return_without_cast_regression)
{
    char const* text = R"(
        contract A {
            function f() public pure returns (uint40) {
                return 20;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    CallState statedata;
    statedata.record(ast);

    ostringstream actual, expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    expect << "struct A Init_A(void)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).model_balance)=(Init_sol_uint256_t(0));";
    expect << "return tmp;";
    expect << "}";
    expect << "sol_uint40_t Method_A_Funcf(struct A*self"
           << ",sol_uint256_t blocknum,sol_address_t sender,sol_uint256_t value)";
    expect << "{";
    expect << "sol_require(((value).v)==(0),0);";
    expect << "return Init_sol_uint40_t(20);";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Checks that payable functions generate the appropriate source..
BOOST_AUTO_TEST_CASE(payable_method)
{
    char const* text = R"(
        contract A {
            function f() public payable returns (uint40) {
                return 20;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    CallState statedata;
    statedata.record(ast);

    ostringstream actual, expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    expect << "struct A Init_A(void)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).model_balance)=(Init_sol_uint256_t(0));";
    expect << "return tmp;";
    expect << "}";
    expect << "sol_uint40_t Method_A_Funcf(struct A*self"
           << ",sol_uint256_t blocknum,sol_address_t sender,sol_uint256_t value)";
    expect << "{";
    expect << "(((self)->model_balance).v)+=((value).v);";
    expect << "return Init_sol_uint40_t(20);";
    expect << "}";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

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

    CallState statedata;
    statedata.record(ast);

    ostringstream actual, expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Init_A
    expect << "struct A Init_A(void)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).model_balance)=(Init_sol_uint256_t(0));";
    expect << "((tmp).user_a)=(Init_sol_uint256_t(0));";
    expect << "((tmp).user_b)=(Init_sol_uint256_t(10));";
    expect << "((tmp).user_c)=(Init_0_A_StructB());";
    expect << "return tmp;";
    expect << "}";
    // -- Init_0_A_StructB
    expect << "struct A_StructB Init_0_A_StructB(void)";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).user_a)=(Init_sol_uint256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_StructB
    expect << "struct A_StructB Init_A_StructB(sol_uint256_t user_a)";
    expect << "{";
    expect << "struct A_StructB tmp=Init_0_A_StructB();";
    expect << "((tmp).user_a)=(user_a);";
    expect << "return tmp;";
    expect << "}";
    // -- A_StructB
    expect << "struct A_StructB ND_A_StructB(void)";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).user_a)=(Init_sol_uint256_t(nd_uint256_t"
           << "(\"Set a in B\")));";
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

    CallState statedata;
    statedata.record(ast);

    ostringstream actual, expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Init_A
    expect << "struct A Init_A(struct A*self,sol_uint256_t blocknum,"
           << "sol_address_t sender,sol_uint256_t value,"
           << "sol_uint256_t user___a)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).model_balance)=(Init_sol_uint256_t(0));";
    expect << "((tmp).user_a)=(Init_sol_uint256_t(0));";
    expect << "((tmp).user_b)=(Init_sol_uint256_t(0));";
    expect << "Ctor_A(&(tmp),blocknum,sender,value,user___a);";
    expect << "return tmp;";
    expect << "}";
    // -- Ctor_A
    expect << "void Ctor_A(struct A*self,sol_uint256_t blocknum,"
           << "sol_address_t sender,sol_uint256_t value"
           << ",sol_uint256_t func_user___a)";
    expect << "{";
    expect << "sol_require(((value).v)==(0),0);";
    expect << "((self->user_a).v)=((func_user___a).v);";
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

    CallState statedata;
    statedata.record(ast);

    ostringstream actual, expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Init_A
    expect << "struct A Init_A(void)";
    expect << "{";
    expect << "struct A tmp;";
    expect << "((tmp).model_balance)=(Init_sol_uint256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_0_A_StructB
    expect << "struct A_StructB Init_0_A_StructB(void)";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_StructB
    expect << "struct A_StructB Init_A_StructB(sol_int256_t user_i1)";
    expect << "{";
    expect << "struct A_StructB tmp=Init_0_A_StructB();";
    expect << "((tmp).user_i1)=(user_i1);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_StructB
    expect << "struct A_StructB ND_A_StructB(void)";
    expect << "{";
    expect << "struct A_StructB tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(nd_int256_t"
           << "(\"Set i1 in B\")));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_0_A_StructC
    expect << "struct A_StructC Init_0_A_StructC(void)";
    expect << "{";
    expect << "struct A_StructC tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(0));";
    expect << "((tmp).user_b1)=(Init_0_A_StructB());";
    expect << "((tmp).user_i2)=(Init_sol_int256_t(0));";
    expect << "((tmp).user_ui1)=(Init_sol_uint256_t(0));";
    expect << "((tmp).user_b2)=(Init_0_A_StructB());";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_StructC
    expect << "struct A_StructC Init_A_StructC(sol_int256_t user_i1"
              ",sol_int256_t user_i2,sol_uint256_t user_ui1)";
    expect << "{";
    expect << "struct A_StructC tmp=Init_0_A_StructC();";
    expect << "((tmp).user_i1)=(user_i1);";
    expect << "((tmp).user_i2)=(user_i2);";
    expect << "((tmp).user_ui1)=(user_ui1);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_StructB
    expect << "struct A_StructC ND_A_StructC(void)";
    expect << "{";
    expect << "struct A_StructC tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(nd_int256_t"
           << "(\"Set i1 in C\")));";
    expect << "((tmp).user_b1)=(ND_A_StructB());";
    expect << "((tmp).user_i2)=(Init_sol_int256_t(nd_int256_t"
           << "(\"Set i2 in C\")));";
    expect << "((tmp).user_ui1)=(Init_sol_uint256_t(nd_uint256_t"
           << "(\"Set ui1 in C\")));";
    expect << "((tmp).user_b2)=(ND_A_StructB());";
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

    CallState statedata;
    statedata.record(ast);

    ostringstream ext_actual, ext_expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::EXT, true
    ).print(ext_actual);
    ext_expect << "struct A Init_A(void);";
    ext_expect << "void Method_A_Funcf(struct A*self,sol_uint256_t blocknum,"
               << "sol_address_t sender,sol_uint256_t value);";

    ostringstream int_actual, int_expect;
    FunctionConverter(
        ast, statedata, converter, 1, FunctionConverter::View::INT, true
    ).print(int_actual);
    int_expect << "struct A_StructB Init_0_A_StructB(void);";
    int_expect << "struct A_StructB Init_A_StructB(sol_int256_t user_i);";
    int_expect << "struct A_StructB ND_A_StructB(void);";
    int_expect << "struct A_Mapm_submap1 Init_0_A_Mapm_submap1(void);";
    int_expect << "struct A_Mapm_submap1 ND_A_Mapm_submap1(void);";
    int_expect << "sol_int256_t Read_A_Mapm_submap1(struct A_Mapm_submap1*arr"
               << ",sol_int256_t key);";
    int_expect << "void Write_A_Mapm_submap1(struct A_Mapm_submap1*arr"
               << ",sol_int256_t key,sol_int256_t dat);";
    int_expect << "sol_int256_t*Ref_A_Mapm_submap1(struct A_Mapm_submap1*arr"
               << ",sol_int256_t key);";
    int_expect << "void Method_A_Funcg(struct A*self,sol_uint256_t blocknum,"
               << "sol_address_t sender,sol_uint256_t value);";

    BOOST_CHECK_EQUAL(ext_actual.str(), ext_expect.str());
    BOOST_CHECK_EQUAL(int_actual.str(), int_expect.str());
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
