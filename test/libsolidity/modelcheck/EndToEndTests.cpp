/**
 * @date 2019
 * Performs end-to-end tests. Test inputs are contracts, and test outputs are
 * all converted components of a C header or body.
 * 
 * These are tests which apply to both ADTConverter and FunctionConverter.
 */

#include <libsolidity/modelcheck/translation/ADT.h>
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

BOOST_FIXTURE_TEST_SUITE(EndToEndTests, ::dev::solidity::test::AnalysisFramework)

// Ensures a single contract with state will generate a single structure type
// with the name of said contract, and an initializer for said structure.
BOOST_AUTO_TEST_CASE(simple_contract)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
		}
	)";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    BOOST_CHECK_EQUAL(adt_actual.str(), "struct A;");
    BOOST_CHECK_EQUAL(func_actual.str(), "struct A Init_A(void);");
}

// Ensures that the non-recursive map case generates the correct structure and
// correct helpers.
BOOST_AUTO_TEST_CASE(simple_map)
{
    char const* text = R"(
        contract A {
            mapping (uint => uint) a;
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_Mapa_submap1;" << "struct A;";
    func_expect << "struct A Init_A(void);";
    func_expect << "struct A_Mapa_submap1 Init_0_A_Mapa_submap1(void);";
    func_expect << "struct A_Mapa_submap1 ND_A_Mapa_submap1(void);";
    func_expect << "sol_uint256_t Read_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "void Write_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*arr,sol_uint256_t key"
                << ",sol_uint256_t dat);";
    func_expect << "sol_uint256_t*Ref_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*arr,sol_uint256_t key);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures a simple structure will generate a new datatype, and that said
// datatype will have an initializer and non-deterministic value generator.
BOOST_AUTO_TEST_CASE(simple_struct)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            struct B {
                uint a;
                uint b;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB;" << "struct A;";
    func_expect << "struct A Init_A(void);";
    func_expect << "struct A_StructB Init_0_A_StructB(void);";
    func_expect << "struct A_StructB Init_A_StructB"
                << "(sol_uint256_t user_a,sol_uint256_t user_b);";
    func_expect << "struct A_StructB ND_A_StructB(void);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_func)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            function simpleFunc(uint _in) public returns (uint _out) {
                _out = _in;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream func_expect;
    func_expect << "struct A Init_A(void);";
    func_expect << "sol_uint256_t Method_A_FuncsimpleFunc"
                << "(struct A*self,struct CallState*state,"
                << "sol_uint256_t func_user___in);";

    BOOST_CHECK_EQUAL(adt_actual.str(), "struct A;");
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that when a function has no return value, its return values are
// assumed to be void.
BOOST_AUTO_TEST_CASE(simple_void_func)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            function simpleFunc(uint _in) public {
                a = _in;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream func_expect;
    func_expect << "struct A Init_A(void);";
    func_expect << "void Method_A_FuncsimpleFunc"
                << "(struct A*self,struct CallState*state,"
                << "sol_uint256_t func_user___in);";

    BOOST_CHECK_EQUAL(adt_actual.str(), "struct A;");
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that maps within structures will generate maps specialized to that
// structure.
BOOST_AUTO_TEST_CASE(struct_nesting)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
            struct B {
                mapping (uint => mapping (uint => uint)) a;
            }
		}
	)";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB_Mapa_submap2;";
    adt_expect << "struct A_StructB_Mapa_submap1;";
    adt_expect << "struct A_StructB;";
    adt_expect << "struct A;";
    func_expect << "struct A Init_A(void);";
    func_expect << "struct A_StructB Init_0_A_StructB(void);";
    func_expect << "struct A_StructB Init_A_StructB(void);";
    func_expect << "struct A_StructB ND_A_StructB(void);";
    func_expect << "struct A_StructB_Mapa_submap1 "
                << "Init_0_A_StructB_Mapa_submap1(void);";
    func_expect << "struct A_StructB_Mapa_submap1 "
                << "ND_A_StructB_Mapa_submap1(void);";
    func_expect << "struct A_StructB_Mapa_submap2 Read_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "void Write_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*arr,sol_uint256_t key"
                << ",struct A_StructB_Mapa_submap2 dat);";
    func_expect << "struct A_StructB_Mapa_submap2*Ref_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "struct A_StructB_Mapa_submap2 Init_0_A_StructB_Mapa_submap2"
                << "(void);";
    func_expect << "struct A_StructB_Mapa_submap2 ND_A_StructB_Mapa_submap2"
                << "(void);";
    func_expect << "sol_uint256_t Read_A_StructB_Mapa_submap2"
                << "(struct A_StructB_Mapa_submap2*arr,sol_uint256_t key);";
    func_expect << "void Write_A_StructB_Mapa_submap2"
                << "(struct A_StructB_Mapa_submap2*arr,sol_uint256_t key"
                << ",sol_uint256_t dat);";
    func_expect << "sol_uint256_t*Ref_A_StructB_Mapa_submap2"
                << "(struct A_StructB_Mapa_submap2*arr,sol_uint256_t key);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Checks that if more than one contract is defined, that each contract will be
// translated.
BOOST_AUTO_TEST_CASE(multiple_contracts)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
            struct B {
                mapping (uint => uint) a;
            }
		}
        contract C {
            uint a;
            mapping (uint => uint) b;
        }
	)";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB_Mapa_submap1;";
    adt_expect << "struct A_StructB;";
    adt_expect << "struct A;";
    adt_expect << "struct C_Mapb_submap1;";
    adt_expect << "struct C;";
    func_expect << "struct A Init_A(void);";
    func_expect << "struct A_StructB Init_0_A_StructB(void);";
    func_expect << "struct A_StructB Init_A_StructB(void);";
    func_expect << "struct A_StructB ND_A_StructB(void);";
    func_expect << "struct A_StructB_Mapa_submap1 Init_0_A_StructB_Mapa_submap1"
                << "(void);";
    func_expect << "struct A_StructB_Mapa_submap1 ND_A_StructB_Mapa_submap1"
                << "(void);";
    func_expect << "sol_uint256_t Read_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "void Write_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*arr,sol_uint256_t key"
                << ",sol_uint256_t dat);";
    func_expect << "sol_uint256_t*Ref_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "struct C Init_C(void);";
    func_expect << "struct C_Mapb_submap1 Init_0_C_Mapb_submap1(void);";
    func_expect << "struct C_Mapb_submap1 ND_C_Mapb_submap1(void);";
    func_expect << "sol_uint256_t Read_C_Mapb_submap1"
                << "(struct C_Mapb_submap1*arr,sol_uint256_t key);";
    func_expect << "void Write_C_Mapb_submap1"
                << "(struct C_Mapb_submap1*arr,sol_uint256_t key"
                << ",sol_uint256_t dat);";
    func_expect << "sol_uint256_t*Ref_C_Mapb_submap1"
                << "(struct C_Mapb_submap1*arr,sol_uint256_t key);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that nested mappings generate the correct number of helper structures
// with the correct names, and that each structure has the correct getter and
// setter methods.
BOOST_AUTO_TEST_CASE(nested_maps)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => mapping (uint => uint))) a;
		}
	)";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_Mapa_submap3;";
    adt_expect << "struct A_Mapa_submap2;";
    adt_expect << "struct A_Mapa_submap1;";
    adt_expect << "struct A;";
    func_expect << "struct A Init_A(void);";
    func_expect << "struct A_Mapa_submap1 Init_0_A_Mapa_submap1(void);";
    func_expect << "struct A_Mapa_submap1 ND_A_Mapa_submap1(void);";
    func_expect << "struct A_Mapa_submap2 Read_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "void Write_A_Mapa_submap1(struct A_Mapa_submap1*arr"
                << ",sol_uint256_t key,struct A_Mapa_submap2 dat);";
    func_expect << "struct A_Mapa_submap2*Ref_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*arr,sol_uint256_t key);";
    func_expect << "struct A_Mapa_submap2 Init_0_A_Mapa_submap2(void);";
    func_expect << "struct A_Mapa_submap2 ND_A_Mapa_submap2(void);";
    func_expect << "struct A_Mapa_submap3 Read_A_Mapa_submap2"
                << "(struct A_Mapa_submap2*arr,sol_uint256_t key);";
    func_expect << "void Write_A_Mapa_submap2(struct A_Mapa_submap2*arr"
                << ",sol_uint256_t key,struct A_Mapa_submap3 dat);";
    func_expect << "struct A_Mapa_submap3*Ref_A_Mapa_submap2"
                << "(struct A_Mapa_submap2*arr,sol_uint256_t key);";
    func_expect << "struct A_Mapa_submap3 Init_0_A_Mapa_submap3(void);";
    func_expect << "struct A_Mapa_submap3 ND_A_Mapa_submap3(void);";
    func_expect << "sol_uint256_t Read_A_Mapa_submap3"
                << "(struct A_Mapa_submap3*arr,sol_uint256_t key);";
    func_expect << "void Write_A_Mapa_submap3"
                << "(struct A_Mapa_submap3*arr,sol_uint256_t key"
                << ",sol_uint256_t dat);";
    func_expect << "sol_uint256_t*Ref_A_Mapa_submap3"
                << "(struct A_Mapa_submap3*arr,sol_uint256_t key);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that returning structures in memory is possible.
BOOST_AUTO_TEST_CASE(nontrivial_retval)
{
    char const* text = R"(
        pragma experimental ABIEncoderV2;
        contract A {
			uint a;
            uint b;
            struct B {
                uint a;
            }
            function advFunc(uint _in) public returns (B memory _out) {
                _out = B(_in);
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, 1, true).print(adt_actual);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB;" << "struct A;";
    func_expect << "struct A Init_A(void);";
    func_expect << "struct A_StructB Init_0_A_StructB(void);";
    func_expect << "struct A_StructB Init_A_StructB(sol_uint256_t user_a);";
    func_expect << "struct A_StructB ND_A_StructB(void);";
    func_expect << "struct A_StructB Method_A_FuncadvFunc"
                << "(struct A*self,struct CallState*state,"
                << "sol_uint256_t func_user___in);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that applying the same visitor twice produces the same results. A
// large contract is used for comprehensive results. Furthermore, this sanity
// checks that the conversion algorithm is not stochastic through some
// implementaiton error.
BOOST_AUTO_TEST_CASE(reproducible)
{
    char const* text = R"(
        contract A {
            struct S { address owner; uint val; }
            uint constant min_amt = 42;
            mapping (uint => S) accs;
            function Open(uint idx) public {
                require(accs[idx].owner == address(0));
                accs[idx] = S(msg.sender, 0);
            }
            function Deposit(uint idx) public payable {
                require(msg.value > min_amt);
                S storage entry = accs[idx];
                if (entry.owner != msg.sender) { Open(idx); }
                entry.val += msg.value;
            }
            function Withdraw(uint idx) public payable {
                require(accs[idx].owner == msg.sender);
                uint amt = accs[idx].val;
                accs[idx] = S(msg.sender, 0);
                assert(accs[idx].val == 0);
                msg.sender.transfer(amt);
            }
            function View(uint idx) public returns (uint amt) {
                amt = accs[idx].val;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_1, adt_2, func_1, func_2;
    ADTConverter(ast, converter, 1, false).print(adt_1);
    ADTConverter(ast, converter, 1, false).print(adt_2);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, false
    ).print(func_1);
    FunctionConverter(
        ast, converter, 1, FunctionConverter::View::FULL, false
    ).print(func_2);

    BOOST_CHECK_EQUAL(adt_1.str(), adt_2.str());
    BOOST_CHECK_EQUAL(func_1.str(), func_2.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
