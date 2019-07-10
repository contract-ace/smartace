/**
 * @date 2019
 * Performs end-to-end tests. Test inputs are contracts, and test outputs are
 * all converted components of a C header or body.
 */

#include <libsolidity/modelcheck/ADTConverter.h>
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
    ModelCheckerForwardDecl,
    ::dev::solidity::test::AnalysisFramework
)

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_a_submap1;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_a_submap1 Init_A_a_submap1();" << endl;
    func_expect << "struct A_a_submap1 ND_A_a_submap1();" << endl;
    func_expect << "unsigned int Read_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void Write_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *Ref_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_B Init_A_B"
                << "(unsigned int a = 0, unsigned int b = 0);" << endl;
    func_expect << "struct A_B ND_A_B();" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that when no arguments are given, a modifier will produce a void
// function, with only state params, and the name `Modifier_<struct>_<func>`.
BOOST_AUTO_TEST_CASE(simple_modifier)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            modifier simpleModifier {
                require(a >= 100, "Placeholder");
                _;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "void Modifier_A_simpleModifier"
                << "(struct A *self, struct CallState *state);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that if a modifier has arguments, that these arguments are added to
// its signature.
BOOST_AUTO_TEST_CASE(modifier_with_args)
{
    char const* text = R"(
        contract A {
            modifier simpleModifier(uint _a, int _b) {
                require(_a >= 100 && _b >= 100,  "Placeholder");
                _;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "void Modifier_A_simpleModifier"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int _a, int _b);" << endl;

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "unsigned int Method_A_simpleFunc"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int _in);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

// Ensures that when functions are pure (as opposed to just views), that said
// function will take no state variables
BOOST_AUTO_TEST_CASE(pure_func)
{
    char const* text = R"(
        contract A {
            function simpleFuncA() public pure returns (uint _out) {
                _out = 4;
            }
            function simpleFuncB() public view returns (uint _out) {
                _out = 4;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "unsigned int Method_A_simpleFuncA();" << endl;
    func_expect << "unsigned int Method_A_simpleFuncB"
                << "(struct A *self, struct CallState *state);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "void Method_A_simpleFunc"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int _in);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    adt_expect << "struct A_B_a_submap1;" << endl;
    adt_expect << "struct A_B_a_submap2;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_B Init_A_B();" << endl;
    func_expect << "struct A_B ND_A_B();" << endl;
    func_expect << "struct A_B_a_submap1 Init_A_B_a_submap1();" << endl;
    func_expect << "struct A_B_a_submap1 ND_A_B_a_submap1();" << endl;
    func_expect << "struct A_B_a_submap2 Read_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);" << endl;
    func_expect << "void Write_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx"
                << ", struct A_B_a_submap2 d);" << endl;
    func_expect << "struct A_B_a_submap2 *Ref_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);" << endl;
    func_expect << "struct A_B_a_submap2 Init_A_B_a_submap2();" << endl;
    func_expect << "struct A_B_a_submap2 ND_A_B_a_submap2();" << endl;
    func_expect << "unsigned int Read_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx);" << endl;
    func_expect << "void Write_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx"
                << ", unsigned int d);" << endl;
    func_expect << "unsigned int *Ref_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx);" << endl;

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    adt_expect << "struct A_B_a_submap1;" << endl;
    adt_expect << "struct C;" << endl;
    adt_expect << "struct C_b_submap1;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_B Init_A_B();" << endl;
    func_expect << "struct A_B ND_A_B();" << endl;
    func_expect << "struct A_B_a_submap1 Init_A_B_a_submap1();" << endl;
    func_expect << "struct A_B_a_submap1 ND_A_B_a_submap1();" << endl;
    func_expect << "unsigned int Read_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void Write_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx"
                << ", unsigned int d);" << endl;
    func_expect << "unsigned int *Ref_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);" << endl;
    func_expect << "struct C Init_C();" << endl;
    func_expect << "struct C_b_submap1 Init_C_b_submap1();" << endl;
    func_expect << "struct C_b_submap1 ND_C_b_submap1();" << endl;
    func_expect << "unsigned int Read_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx);" << endl;
    func_expect << "void Write_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx"
                << ", unsigned int d);" << endl;
    func_expect << "unsigned int *Ref_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx);" << endl;

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_a_submap1;" << endl;
    adt_expect << "struct A_a_submap2;" << endl;
    adt_expect << "struct A_a_submap3;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_a_submap1 Init_A_a_submap1();" << endl;
    func_expect << "struct A_a_submap1 ND_A_a_submap1();" << endl;
    func_expect << "struct A_a_submap2 Read_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);" << endl;
    func_expect << "void Write_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx"
                << ", struct A_a_submap2 d);" << endl;
    func_expect << "struct A_a_submap2 *Ref_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);" << endl;
    func_expect << "struct A_a_submap2 Init_A_a_submap2();" << endl;
    func_expect << "struct A_a_submap2 ND_A_a_submap2();" << endl;
    func_expect << "struct A_a_submap3 Read_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx);" << endl;
    func_expect << "void Write_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx"
                << ", struct A_a_submap3 d);" << endl;
    func_expect << "struct A_a_submap3 *Ref_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx);" << endl;
    func_expect << "struct A_a_submap3 Init_A_a_submap3();" << endl;
    func_expect << "struct A_a_submap3 ND_A_a_submap3();" << endl;
    func_expect << "unsigned int Read_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx);" << endl;
    func_expect << "void Write_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx"
                << ", unsigned int d);" << endl;
    func_expect << "unsigned int *Ref_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx);" << endl;

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_B Init_A_B(unsigned int a = 0);" << endl;
    func_expect << "struct A_B ND_A_B();" << endl;
    func_expect << "struct A_B Method_A_advFunc"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int _in);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    ostringstream actual, expect;
    FunctionConverter(ast, converter, false).print(actual);
    expect << "struct A Init_A()" << endl;
    expect << "{" << endl;
    expect << "struct A tmp;" << endl;
    expect << "tmp.d_a = 0;" << endl;
    expect << "tmp.d_b = 10;" << endl;
    expect << "tmp.d_c = Init_A_B();" << endl;
    expect << "return tmp;" << endl;
    expect << "}" << endl;
    expect << "struct A_B Init_A_B(unsigned int a = 0)" << endl;
    expect << "{" << endl;
    expect << "struct A_B tmp;" << endl;
    expect << "tmp.d_a = a;" << endl;
    expect << "return tmp;" << endl;
    expect << "}" << endl;
    expect << "struct A_B ND_A_B();" << endl;

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
    expect << "struct A Init_A(struct A *self, struct CallState *state"
           << ", unsigned int _a)" << endl;
    expect << "{" << endl;
    expect << "struct A tmp;" << endl;
    expect << "tmp.d_a = 0;" << endl;
    expect << "tmp.d_b = 0;" << endl;
    expect << "Ctor_A(&tmp, state, _a);" << endl;
    expect << "return tmp;" << endl;
    expect << "}" << endl;
    expect << "void Ctor_A(struct A *self, struct CallState *state"
           << ", unsigned int _a)" << endl;
    expect << "{" << endl;
    expect << "(self->d_a)=(_a);" << endl;
    expect << "}" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// Attempts a full translation of a contract which highlights most features of
// the model, in a single contract context.
BOOST_AUTO_TEST_CASE(full_declaration)
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
                // S storage entry = accs[idx];
                // if (entry.owner != msg.sender) { Open(idx); }
                // entry.val += msg.value;
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

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, false).print(adt_actual);
    FunctionConverter(ast, converter, false).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A" << endl;
    adt_expect << "{" << endl;
    adt_expect << "unsigned int d_min_amt;" << endl;
    adt_expect << "struct A_accs_submap1 d_accs;" << endl;
    adt_expect << "};" << endl;
    adt_expect << "struct A_S" << endl;
    adt_expect << "{" << endl;
    adt_expect << "int d_owner;" << endl;
    adt_expect << "unsigned int d_val;" << endl;
    adt_expect << "};" << endl;
    adt_expect << "struct A_accs_submap1" << endl;
    adt_expect << "{" << endl;
    adt_expect << "int m_set;" << endl;
    adt_expect << "unsigned int m_curr;" << endl;
    adt_expect << "struct A_S d_;" << endl;
    adt_expect << "struct A_S d_nd;" << endl;
    adt_expect << "};" << endl;
    func_expect << "struct A Init_A()" << endl;
    func_expect << "{" << endl;
    func_expect << "struct A tmp;" << endl;
    func_expect << "tmp.d_min_amt = 42;" << endl;
    func_expect << "tmp.d_accs = Init_A_accs_submap1();" << endl;
    func_expect << "return tmp;" << endl;
    func_expect << "}" << endl;
    func_expect << "struct A_S Init_A_S(int owner = 0"
                << ", unsigned int val = 0)" << endl;
    func_expect << "{" << endl;
    func_expect << "struct A_S tmp;" << endl;
    func_expect << "tmp.d_owner = owner;" << endl;
    func_expect << "tmp.d_val = val;" << endl;
    func_expect << "return tmp;" << endl;
    func_expect << "}" << endl;
    func_expect << "struct A_S ND_A_S();" << endl;
    func_expect << "struct A_accs_submap1 Init_A_accs_submap1()" << endl;
    func_expect << "{" << endl;
    func_expect << "struct A_accs_submap1 tmp;" << endl;
    func_expect << "tmp.m_set = 0;" << endl;
    func_expect << "tmp.m_curr = 0;" << endl;
    func_expect << "tmp.d_ = Init_A_S();" << endl;
    func_expect << "tmp.d_nd = Init_A_S();" << endl;
    func_expect << "return tmp;" << endl;
    func_expect << "}" << endl;
    func_expect << "struct A_accs_submap1 ND_A_accs_submap1();" << endl;
    func_expect << "struct A_S Read_A_accs_submap1"
                << "(struct A_accs_submap1 *a, unsigned int idx)" << endl;
    func_expect << "{" << endl;
    func_expect << "if (a->m_set == 0) "
                << "{ a->m_curr = idx; a->m_set = 1; }" << endl;
    func_expect << "if (idx != a->m_curr) return ND_A_S();" << endl;
    func_expect << "return a->d_;" << endl;
    func_expect << "}" << endl;
    func_expect << "void Write_A_accs_submap1"
                << "(struct A_accs_submap1 *a, unsigned int idx"
                << ", struct A_S d)" << endl;
    func_expect << "{" << endl;
    func_expect << "if (a->m_set == 0) "
                << "{ a->m_curr = idx; a->m_set = 1; }" << endl;
    func_expect << "if (idx == a->m_curr) { a->d_ = d; }" << endl;
    func_expect << "}" << endl;
    func_expect << "struct A_S *Ref_A_accs_submap1"
                << "(struct A_accs_submap1 *a, unsigned int idx)" << endl;
    func_expect << "{" << endl;
    func_expect << "if (a->m_set == 0) "
                << "{ a->m_curr = idx; a->m_set = 1; }" << endl;
    func_expect << "if (idx != a->m_curr)" << endl;
    func_expect << "{" << endl;
    func_expect << "a->d_nd = ND_A_S();" << endl;
    func_expect << "return &a->d_nd;" << endl;
    func_expect << "}" << endl;
    func_expect << "return &a->d_;" << endl;
    func_expect << "}" << endl;
    func_expect << "void Method_A_Open"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int idx)" << endl;
    func_expect << "{" << endl;
    func_expect << "assume(((Read_A_accs_submap1(&(self->d_accs), idx)).d_owner"
                << ")==(((int)(0))));" << endl;
    func_expect << "Write_A_accs_submap1(&(self->d_accs), idx"
                << ", (Init_A_S(state->sender, 0)));" << endl;
    func_expect << "}" << endl;
    func_expect << "void Method_A_Deposit"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int idx)" << endl;
    func_expect << "{" << endl;
    func_expect << "assume((state->value)>(self->d_min_amt));" << endl;
    func_expect << "}" << endl;
    func_expect << "void Method_A_Withdraw"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int idx)" << endl;
    func_expect << "{" << endl;
    func_expect << "assume(((Read_A_accs_submap1(&(self->d_accs), idx)).d_owner"
                << ")==(state->sender));" << endl;
    func_expect << "unsigned int amt = "
                << "(Read_A_accs_submap1(&(self->d_accs), idx)).d_val;" << endl;
    func_expect << "Write_A_accs_submap1(&(self->d_accs), idx"
                << ", (Init_A_S(state->sender, 0)));" << endl;
    func_expect << "assert(((Read_A_accs_submap1(&(self->d_accs), idx)).d_val"
                << ")==(0));" << endl;
    func_expect << "_pay(state, state->sender, amt);" << endl;
    func_expect << "}" << endl;
    func_expect << "unsigned int Method_A_View"
                << "(struct A *self, struct CallState *state"
                << ", unsigned int idx)" << endl;
    func_expect << "{" << endl;
    func_expect << "unsigned int amt;" << endl;
    func_expect << "(amt)=((Read_A_accs_submap1("
                << "&(self->d_accs), idx)).d_val);" << endl;
    func_expect << "return amt;" << endl;
    func_expect << "}" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());

}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
