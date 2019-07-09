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

BOOST_FIXTURE_TEST_SUITE(ModelCheckerForwardDecl, ::dev::solidity::test::AnalysisFramework)

BOOST_AUTO_TEST_CASE(simple_contract)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
		}
	)";

    const auto &ast = *parseAndAnalyse(text);

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

BOOST_AUTO_TEST_CASE(simple_map)
{
    char const* text = R"(
        contract A {
            mapping (uint => uint) a;
        }
    )";

    const auto &ast = *parseAndAnalyse(text);

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
    func_expect << "unsigned int "
                << "Read_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "struct A_B Init_A_B(unsigned int a = 0, unsigned int b = 0);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "void Modifier_A_simpleModifier(struct A *self, struct CallState *state);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "void Modifier_A_simpleModifier(struct A *self, struct CallState *state, unsigned int _a, int _b);" << endl;

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "unsigned int Method_A_simpleFunc(struct A *self, struct CallState *state, unsigned int _in);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(pure_func)
{
    char const* text = R"(
        contract A {
            function simpleFunc() public pure returns (uint _out) {
                _out = 4;
            }
        }
    )";

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "unsigned int Method_A_simpleFunc();" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A();" << endl;
    func_expect << "void Method_A_simpleFunc(struct A *self, struct CallState *state, unsigned int _in);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

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
    func_expect << "struct A_B_a_submap1 Init_A_B_a_submap1();" << endl;
    func_expect << "struct A_B_a_submap2 "
                << "Read_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx, struct A_B_a_submap2 d);"
                << endl;
    func_expect << "struct A_B_a_submap2 *"
                << "Ref_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "struct A_B_a_submap2 Init_A_B_a_submap2();" << endl;
    func_expect << "unsigned int "
                << "Read_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

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
    func_expect << "struct A_B_a_submap1 Init_A_B_a_submap1();" << endl;
    func_expect << "unsigned int "
                << "Read_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "struct C Init_C();" << endl;
    func_expect << "struct C_b_submap1 Init_C_b_submap1();" << endl;
    func_expect << "unsigned int "
                << "Read_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(nested_maps)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => mapping (uint => uint))) a;
		}
	)";

    const auto &ast = *parseAndAnalyse(text);

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
    func_expect << "struct A_a_submap2 "
                << "Read_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx, struct A_a_submap2 d);"
                << endl;
    func_expect << "struct A_a_submap2 *"
                << "Ref_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "struct A_a_submap2 Init_A_a_submap2();" << endl;
    func_expect << "struct A_a_submap3 "
                << "Read_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx, struct A_a_submap3 d);"
                << endl;
    func_expect << "struct A_a_submap3 *"
                << "Ref_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "struct A_a_submap3 Init_A_a_submap3();" << endl;
    func_expect << "unsigned int "
                << "Read_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(custom_ctor)
{
    char const* text = R"(
		contract A {
            uint a;
			constructor(uint _a) public {
                a = _a;
            }
		}
	)";

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(ast, converter, true).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Init_A(struct A *self, struct CallState *state, unsigned int _a);" << endl;
    func_expect << "void Ctor_A(struct A *self, struct CallState *state, unsigned int _a);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

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
    func_expect << "struct A_B Method_A_advFunc(struct A *self, struct CallState *state, unsigned int _in);" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    FunctionConverter(ast, converter, false).print(actual);
    expect << "struct A Init_A()" << endl
           << "{" << endl
           << "struct A tmp;" << endl
           << "tmp.d_a = 0;" << endl
           << "tmp.d_b = 10;" << endl
           << "tmp.d_c = Init_A_B();" << endl
           << "return tmp;" << endl
           << "}" << endl
           << "struct A_B Init_A_B(unsigned int a = 0)" << endl
           << "{" << endl
           << "struct A_B tmp;" << endl
           << "tmp.d_a = a;" << endl
           << "return tmp;" << endl
           << "}" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

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

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream actual, expect;
    FunctionConverter(ast, converter, false).print(actual);
    expect << "struct A Init_A(struct A *self, struct CallState *state, unsigned int _a)" << endl
           << "{" << endl
           << "struct A tmp;" << endl
           << "tmp.d_a = 0;" << endl
           << "tmp.d_b = 0;" << endl
           << "Ctor_A(&tmp, state, _a);" << endl
           << "return tmp;" << endl
           << "}" << endl
           << "void Ctor_A(struct A *self, struct CallState *state, unsigned int _a)" << endl
           << "{" << endl
           << "(self->d_a)=(_a);" << endl
           << "}" << endl;

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
                // require(accs[idx].owner == address(0));
                // accs[idx] = S(msg.sender, 0);
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
                // accs[idx] = S(msg.sender, 0);
                // assert(accs[idx].val == 0);
                msg.sender.transfer(amt);
            }
            function View(uint idx) public returns (uint amt) {
                amt = accs[idx].val;
            }
        }
    )";

    const auto &ast = *parseAndAnalyse(text);

    TypeConverter converter;
    converter.record(ast);

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, false).print(adt_actual);
    FunctionConverter(ast, converter, false).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A" << endl
               << "{" << endl
               << "unsigned int d_min_amt;" << endl
               << "struct A_accs_submap1 d_accs;" << endl
               << "};" << endl
               << "struct A_S" << endl
               << "{" << endl
               << "int d_owner;" << endl
               << "unsigned int d_val;" << endl
               << "};" << endl
               << "struct A_accs_submap1" << endl
               << "{" << endl
               << "int m_set;" << endl
               << "unsigned int m_curr;" << endl
               << "struct A_S d_;" << endl
               << "};" << endl;
    func_expect << "struct A Init_A()" << endl
                << "{" << endl
                << "struct A tmp;" << endl
                << "tmp.d_min_amt = 42;" << endl
                << "tmp.d_accs = Init_A_accs_submap1();" << endl
                << "return tmp;" << endl
                << "}" << endl
                << "struct A_S Init_A_S(int owner = 0, unsigned int val = 0)" << endl
                << "{" << endl
                << "struct A_S tmp;" << endl
                << "tmp.d_owner = owner;" << endl
                << "tmp.d_val = val;" << endl
                << "return tmp;" << endl
                << "}" << endl
                << "struct A_accs_submap1 Init_A_accs_submap1();" << endl
                << "struct A_S Read_A_accs_submap1(struct A_accs_submap1 *a, unsigned int idx);" << endl
                << "void Write_A_accs_submap1(struct A_accs_submap1 *a, unsigned int idx, struct A_S d);" << endl
                << "struct A_S *Ref_A_accs_submap1(struct A_accs_submap1 *a, unsigned int idx);" << endl
                << "void Method_A_Open(struct A *self, struct CallState *state, unsigned int idx)" << endl
                << "{" << endl
                << "}" << endl
                << "void Method_A_Deposit(struct A *self, struct CallState *state, unsigned int idx)" << endl
                << "{" << endl
                << "assume((state->value)>(self->d_min_amt));" << endl
                << "}" << endl
                << "void Method_A_Withdraw(struct A *self, struct CallState *state, unsigned int idx)" << endl
                << "{" << endl
                << "assume(((Read_A_accs_submap1(&(self->d_accs), idx)).d_owner)==(state->sender));" << endl
                << "unsigned int amt = (Read_A_accs_submap1(&(self->d_accs), idx)).d_val;" << endl
                << "_pay(state, state->sender, amt);" << endl
                << "}" << endl
                << "unsigned int Method_A_View(struct A *self, struct CallState *state, unsigned int idx)" << endl
                << "{" << endl
                << "unsigned int amt;" << endl
                << "(amt)=((Read_A_accs_submap1(&(self->d_accs), idx)).d_val);" << endl
                << "return amt;" << endl
                << "}" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());

}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
