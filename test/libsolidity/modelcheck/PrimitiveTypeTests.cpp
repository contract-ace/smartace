/**
 * @date 2019
 * Targets libsolidity/modelcheck/PrimitiveTypeGenerator.{h,cpp} on contracts.
 * 
 * Detection test targets the presence of a primitively typed map key, map
 * value, state variable, local variable, modifier parameter, and function
 * parameter. Formatting tests ensure that once the detection flag has been
 * thrown, the generated header will define the corresponding type.
 * 
 * An integration test will look at analyzing a more detailed complex with mixed
 * types.
 */

#include <libsolidity/modelcheck/PrimitiveTypeGenerator.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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
    PrimitiveTypes,
    ::dev::solidity::test::AnalysisFramework
)

// Targeted test for boolean typed values.
BOOST_AUTO_TEST_CASE(boolean_detection)
{
    char const* text = R"(
        contract A {
            bool v1;
            mapping(bool => int) v2;
            mapping(int => bool) v3;
            function f1() public { bool v; }
            function f2(bool v) public { }
            function f3() public returns (bool) { return false; }
            modifier m1(bool v) { _; }
        }
        contract B {
            int v1;
            mapping(int => int) v2;
            function f3(int _v) public returns (int) { return _v; }
            modifier m1(int v) { _; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    for (auto node : ctrt_a.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        BOOST_CHECK(gen.found_bool());
    }
    for (auto node : ctrt_b.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        BOOST_CHECK(!gen.found_bool());
    }
}

// Targeted test for address typed values.
BOOST_AUTO_TEST_CASE(address_detection)
{
    char const* text = R"(
        contract A {
            address v1;
            mapping(bool => address) v2;
            mapping(address => bool) v3;
            function f1() public { address v; }
            function f2(address v) public { }
            function f3() public returns (address) { return address(1); }
            modifier m1(address v) { _; }
        }
        contract B {
            int v1;
            mapping(int => int) v2;
            function f3(int _v) public returns (int) { return _v; }
            modifier m1(int v) { _; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    for (auto node : ctrt_a.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        BOOST_CHECK(gen.found_address());
    }
    for (auto node : ctrt_b.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        BOOST_CHECK(!gen.found_address());
    }
}

// Targeted test for int typed values.
BOOST_AUTO_TEST_CASE(int_detection)
{
    char const* text = R"(
        contract A {
            int32 v1;
            mapping(bool => int32) v2;
            mapping(int32 => bool) v3;
            function f1() public { int32 v; }
            function f2(int32 v) public { }
            function f3() public returns (int32) { return 1; }
            modifier m1(int32 v) { _; }
        }
        contract B {
            int24 v1;
            mapping(int24 => address) v2;
            mapping(address => int24) v3;
            function f1() public { int24 v; }
            function f2(int24 v) public { }
            function f3() public returns (int24) { return 1; }
            modifier m1(int24 v) { _; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    for (auto node : ctrt_a.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            BOOST_CHECK_EQUAL(gen.found_int(i), i == 4);
            BOOST_CHECK(!gen.found_uint(i));
        }
    }
    for (auto node : ctrt_b.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            BOOST_CHECK_EQUAL(gen.found_int(i), i == 3);
            BOOST_CHECK(!gen.found_uint(i));
        }
    }
}

// Targeted test for uint typed values.
BOOST_AUTO_TEST_CASE(uint_detection)
{
    char const* text = R"(
        contract A {
            uint32 v1;
            mapping(bool => uint32) v2;
            mapping(uint32 => bool) v3;
            function f1() public { uint32 v; }
            function f2(uint32 v) public { }
            function f3() public returns (uint32) { return 1; }
            modifier m1(uint32 v) { _; }
        }
        contract B {
            uint24 v1;
            mapping(uint24 => address) v2;
            mapping(address => uint24) v3;
            function f1() public { uint24 v; }
            function f2(uint24 v) public { }
            function f3() public returns (uint24) { return 1; }
            modifier m1(uint24 v) { _; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    for (auto node : ctrt_a.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            BOOST_CHECK_EQUAL(gen.found_uint(i), i == 4);
            BOOST_CHECK(!gen.found_int(i));
        }
    }
    for (auto node : ctrt_b.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            BOOST_CHECK_EQUAL(gen.found_uint(i), i == 3);
            BOOST_CHECK(!gen.found_int(i));
        }
    }
}

// Targeted test for fixed typed values.
BOOST_AUTO_TEST_CASE(fixed_detection)
{
    char const* text = R"(
        contract A {
            fixed32x11 v1;
            mapping(bool => fixed32x11) v2;
            mapping(fixed32x11 => bool) v3;
            function f1() public { fixed32x11 v; }
            function f2(fixed32x11 v) public { }
            function f3() public returns (fixed32x11 r) { }
            modifier m1(fixed32x11 v) { _; }
        }
        contract B {
            fixed24x10 v1;
            mapping(fixed24x10 => address) v2;
            mapping(address => fixed24x10) v3;
            function f1() public { fixed24x10 v; }
            function f2(fixed24x10 v) public { }
            function f3() public returns (fixed24x10 r) { }
            modifier m1(fixed24x10 v) { _; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    for (auto node : ctrt_a.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            for (unsigned char j = 0; j <= 80; ++j)
            {
                BOOST_CHECK_EQUAL(gen.found_fixed(i, j), i == 4 && j == 11);
                BOOST_CHECK(!gen.found_ufixed(i, j));
            }
        }
    }
    for (auto node : ctrt_b.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            for (unsigned char j = 0; j <= 80; ++j)
            {
                BOOST_CHECK_EQUAL(gen.found_fixed(i, j), i == 3 && j == 10);
                BOOST_CHECK(!gen.found_ufixed(i, j));
            }
        }
    }
}

// Targeted test for fixed typed values.
BOOST_AUTO_TEST_CASE(ufixed_detection)
{
    char const* text = R"(
        contract A {
            ufixed32x11 v1;
            mapping(bool => ufixed32x11) v2;
            mapping(ufixed32x11 => bool) v3;
            function f1() public { ufixed32x11 v; }
            function f2(ufixed32x11 v) public { }
            function f3() public returns (ufixed32x11 r) { }
            modifier m1(ufixed32x11 v) { _; }
        }
        contract B {
            ufixed24x10 v1;
            mapping(ufixed24x10 => address) v2;
            mapping(address => ufixed24x10) v3;
            function f1() public { ufixed24x10 v; }
            function f2(ufixed24x10 v) public { }
            function f3() public returns (ufixed24x10 r) { }
            modifier m1(ufixed24x10 v) { _; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    for (auto node : ctrt_a.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            for (unsigned char j = 0; j <= 80; ++j)
            {
                BOOST_CHECK_EQUAL(gen.found_ufixed(i, j), i == 4 && j == 11);
                BOOST_CHECK(!gen.found_fixed(i, j));
            }
        }
    }
    for (auto node : ctrt_b.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);
        for (unsigned char i = 1; i <= 32; ++i)
        {
            for (unsigned char j = 0; j <= 80; ++j)
            {
                BOOST_CHECK_EQUAL(gen.found_ufixed(i, j), i == 3 && j == 10);
                BOOST_CHECK(!gen.found_fixed(i, j));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(bool_formatting)
{
    char const* text = R"( contract A { bool v1; } )";
    auto const& ast = *parseAndAnalyse(text);

    PrimitiveTypeGenerator gen(ast);

    ostringstream actual, expected;
    gen.print(actual);
    expected << "#pragma once" << endl;
    expected << "#include <stdint.h>" << endl;
    expected << "struct bool{uint8_t v;};";
    expected << "typedef struct bool bool_t;";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(address_formatting)
{
    char const* text = R"( contract A { address v1; } )";
    auto const& ast = *parseAndAnalyse(text);

    PrimitiveTypeGenerator gen(ast);

    ostringstream actual, expected;
    gen.print(actual);
    expected << "#pragma once" << endl;
    expected << "#include <stdint.h>" << endl;
    expected << "struct address{uint64_t v;};";
    expected << "typedef struct address address_t;";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(ints_small_aligned_formatting)
{
    char const* text = R"(
        contract A {
            int8 v1;
            int64 v2;
            uint8 v3;
            uint64 v4;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    for (auto node : ctrt.subNodes())
    {
        PrimitiveTypeGenerator gen(*node);

        ostringstream actual, expected;
        gen.print(actual);
        expected << "#pragma once" << endl;
        expected << "#include <stdint.h>" << endl;
        BOOST_CHECK_EQUAL(actual.str(), expected.str());
    }
}

BOOST_AUTO_TEST_CASE(ints_small_misaligned_formatting)
{
    char const* text = R"(
        contract A {
            int24 v1;
            int40 v2;
            uint24 v3;
            uint40 v4;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    {
        ostringstream s24_actual, s24_expected;
        PrimitiveTypeGenerator s24(*ctrt.stateVariables()[0]);
        s24.print(s24_actual);
        s24_expected << "#pragma once" << endl;
        s24_expected << "#include <stdint.h>" << endl;
        s24_expected << "struct int24{int32_t v;};";
        s24_expected << "typedef struct int24 int24_t;";
        BOOST_CHECK_EQUAL(s24_actual.str(), s24_expected.str());
    }
    {
        ostringstream s40_actual, s40_expected;
        PrimitiveTypeGenerator s40(*ctrt.stateVariables()[1]);
        s40.print(s40_actual);
        s40_expected << "#pragma once" << endl;
        s40_expected << "#include <stdint.h>" << endl;
        s40_expected << "struct int40{int64_t v;};";
        s40_expected << "typedef struct int40 int40_t;";
        BOOST_CHECK_EQUAL(s40_actual.str(), s40_expected.str());
    }
    {
        ostringstream u24_actual, u24_expected;
        PrimitiveTypeGenerator u24(*ctrt.stateVariables()[2]);
        u24.print(u24_actual);
        u24_expected << "#pragma once" << endl;
        u24_expected << "#include <stdint.h>" << endl;
        u24_expected << "struct uint24{uint32_t v;};";
        u24_expected << "typedef struct uint24 uint24_t;";
        BOOST_CHECK_EQUAL(u24_actual.str(), u24_expected.str());
    }
    {
        ostringstream u40_actual, u40_expected;
        PrimitiveTypeGenerator u40(*ctrt.stateVariables()[3]);
        u40.print(u40_actual);
        u40_expected << "#pragma once" << endl;
        u40_expected << "#include <stdint.h>" << endl;
        u40_expected << "struct uint40{uint64_t v;};";
        u40_expected << "typedef struct uint40 uint40_t;";
        BOOST_CHECK_EQUAL(u40_actual.str(), u40_expected.str());
    }
}

// TODO(scottwe): Unsigned integers with power-of-two bits over 64.
// TODO(scottwe): Signed integers with power-of-two bits over 64.
// TODO(scottwe): Unsigned integers with non-power-of-two-bits over 64.
// TODO(scottwe): Signed integers with non-power-of-two-bits over 64.

BOOST_AUTO_TEST_CASE(fixeds_small_aligned_formatting)
{
    char const* text = R"(
        contract A {
            fixed8x10 v1;
            fixed64x11 v2;
            ufixed8x10 v3;
            ufixed64x11 v4;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    {
        ostringstream s8x10_actual, s8x10_expected;
        PrimitiveTypeGenerator s8x10(*ctrt.stateVariables()[0]);
        s8x10.print(s8x10_actual);
        s8x10_expected << "#pragma once" << endl;
        s8x10_expected << "#include <stdint.h>" << endl;
        s8x10_expected << "struct fixed8x10{int8_t v;};";
        s8x10_expected << "typedef struct fixed8x10 fixed8x10_t;";
        BOOST_CHECK_EQUAL(s8x10_actual.str(), s8x10_expected.str());
    }
    {
        ostringstream s64x11_actual, s64x11_expected;
        PrimitiveTypeGenerator s64x11(*ctrt.stateVariables()[1]);
        s64x11.print(s64x11_actual);
        s64x11_expected << "#pragma once" << endl;
        s64x11_expected << "#include <stdint.h>" << endl;
        s64x11_expected << "struct fixed64x11{int64_t v;};";
        s64x11_expected << "typedef struct fixed64x11 fixed64x11_t;";
        BOOST_CHECK_EQUAL(s64x11_actual.str(), s64x11_expected.str());
    }
    {
        ostringstream u8x10_actual, u8x10_expected;
        PrimitiveTypeGenerator u8x10(*ctrt.stateVariables()[2]);
        u8x10.print(u8x10_actual);
        u8x10_expected << "#pragma once" << endl;
        u8x10_expected << "#include <stdint.h>" << endl;
        u8x10_expected << "struct ufixed8x10{uint8_t v;};";
        u8x10_expected << "typedef struct ufixed8x10 ufixed8x10_t;";
        BOOST_CHECK_EQUAL(u8x10_actual.str(), u8x10_expected.str());
    }
    {
        ostringstream u64x11_actual, u64x11_expected;
        PrimitiveTypeGenerator u64x11(*ctrt.stateVariables()[3]);
        u64x11.print(u64x11_actual);
        u64x11_expected << "#pragma once" << endl;
        u64x11_expected << "#include <stdint.h>" << endl;
        u64x11_expected << "struct ufixed64x11{uint64_t v;};";
        u64x11_expected << "typedef struct ufixed64x11 ufixed64x11_t;";
        BOOST_CHECK_EQUAL(u64x11_actual.str(), u64x11_expected.str());
    }
}

BOOST_AUTO_TEST_CASE(fixeds_small_misaligned_formatting)
{
    char const* text = R"(
        contract A {
            fixed24x10 v1;
            fixed40x11 v2;
            ufixed24x10 v3;
            ufixed40x11 v4;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    {
        ostringstream s24x10_actual, s24x10_expected;
        PrimitiveTypeGenerator s24x10(*ctrt.stateVariables()[0]);
        s24x10.print(s24x10_actual);
        s24x10_expected << "#pragma once" << endl;
        s24x10_expected << "#include <stdint.h>" << endl;
        s24x10_expected << "struct fixed24x10{int32_t v;};";
        s24x10_expected << "typedef struct fixed24x10 fixed24x10_t;";
        BOOST_CHECK_EQUAL(s24x10_actual.str(), s24x10_expected.str());
    }
    {
        ostringstream s40x11_actual, s40x11_expected;
        PrimitiveTypeGenerator s40x11(*ctrt.stateVariables()[1]);
        s40x11.print(s40x11_actual);
        s40x11_expected << "#pragma once" << endl;
        s40x11_expected << "#include <stdint.h>" << endl;
        s40x11_expected << "struct fixed40x11{int64_t v;};";
        s40x11_expected << "typedef struct fixed40x11 fixed40x11_t;";
        BOOST_CHECK_EQUAL(s40x11_actual.str(), s40x11_expected.str());
    }
    {
        ostringstream u24x10_actual, u24x10_expected;
        PrimitiveTypeGenerator u24x10(*ctrt.stateVariables()[2]);
        u24x10.print(u24x10_actual);
        u24x10_expected << "#pragma once" << endl;
        u24x10_expected << "#include <stdint.h>" << endl;
        u24x10_expected << "struct ufixed24x10{uint32_t v;};";
        u24x10_expected << "typedef struct ufixed24x10 ufixed24x10_t;";
        BOOST_CHECK_EQUAL(u24x10_actual.str(), u24x10_expected.str());
    }
    {
        ostringstream u40x11_actual, u40x11_expected;
        PrimitiveTypeGenerator u40x11(*ctrt.stateVariables()[3]);
        u40x11.print(u40x11_actual);
        u40x11_expected << "#pragma once" << endl;
        u40x11_expected << "#include <stdint.h>" << endl;
        u40x11_expected << "struct ufixed40x11{uint64_t v;};";
        u40x11_expected << "typedef struct ufixed40x11 ufixed40x11_t;";
        BOOST_CHECK_EQUAL(u40x11_actual.str(), u40x11_expected.str());
    }
}

// TODO(scottwe): Unsigned fixed-point with power-of-two bits over 64.
// TODO(scottwe): Signed fixed-point with power of two bits over 64.
// TODO(scottwe): Unsigned fixed-point with non-power-of-two-bits over 64.
// TODO(scottwe): Signed fixed-point with non-power-of-two-bits over 64.

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
