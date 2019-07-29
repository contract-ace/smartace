/**
 * @date 2019
 * Targets libsolidity/modelcheck/PrimitiveTypeGenerator.{h,cpp} on contracts.
 * 
 * Each test targets the presence of a primitively typed map key, map value,
 * state variable, local variable, modifier parameter, and function parameter.
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

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
