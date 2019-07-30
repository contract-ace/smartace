/**
 * @date 2019
 * Performs end-to-end tests. Test inputs are contracts, and test outputs are
 * all converted components of a C header or body.
 * 
 * These are tests which apply to both ADTConverter and FunctionConverter.
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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    BOOST_CHECK_EQUAL(adt_actual.str(), "struct A;");
    BOOST_CHECK_EQUAL(func_actual.str(), "struct A Init_A();");
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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_Mapa_submap1;" << "struct A;";
    func_expect << "struct A Init_A();";
    func_expect << "struct A_Mapa_submap1 Init_0_A_Mapa_submap1();";
    func_expect << "struct A_Mapa_submap1 ND_A_Mapa_submap1();";
    func_expect << "unsigned int Read_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*a,unsigned int idx);";
    func_expect << "void Write_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*a,unsigned int idx,unsigned int d);";
    func_expect << "unsigned int*Ref_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*a,unsigned int idx);";

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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB;" << "struct A;";
    func_expect << "struct A Init_A();";
    func_expect << "struct A_StructB Init_0_A_StructB();";
    func_expect << "struct A_StructB Init_A_StructB(unsigned int a,unsigned int b);";
    func_expect << "struct A_StructB ND_A_StructB();";

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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream func_expect;
    func_expect << "struct A Init_A();";
    func_expect << "unsigned int Method_A_FuncsimpleFunc"
                << "(struct A*self,struct CallState*state,unsigned int _in);";

    BOOST_CHECK_EQUAL(adt_actual.str(), "struct A;");
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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream func_expect;
    func_expect << "struct A Init_A();";
    func_expect << "unsigned int Method_A_FuncsimpleFuncA();";
    func_expect << "unsigned int Method_A_FuncsimpleFuncB"
                << "(struct A*self,struct CallState*state);";

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream func_expect;
    func_expect << "struct A Init_A();";
    func_expect << "void Method_A_FuncsimpleFunc"
                << "(struct A*self,struct CallState*state,unsigned int _in);";

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
    ADTConverter(ast, converter, true).print(adt_actual);
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB_Mapa_submap2;";
    adt_expect << "struct A_StructB_Mapa_submap1;";
    adt_expect << "struct A_StructB;";
    adt_expect << "struct A;";
    func_expect << "struct A Init_A();";
    func_expect << "struct A_StructB Init_0_A_StructB();";
    func_expect << "struct A_StructB Init_A_StructB();";
    func_expect << "struct A_StructB ND_A_StructB();";
    func_expect << "struct A_StructB_Mapa_submap1 Init_0_A_StructB_Mapa_submap1();";
    func_expect << "struct A_StructB_Mapa_submap1 ND_A_StructB_Mapa_submap1();";
    func_expect << "struct A_StructB_Mapa_submap2 Read_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*a,unsigned int idx);";
    func_expect << "void Write_A_StructB_Mapa_submap1(struct A_StructB_Mapa_submap1*a"
                << ",unsigned int idx,struct A_StructB_Mapa_submap2 d);";
    func_expect << "struct A_StructB_Mapa_submap2*Ref_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*a,unsigned int idx);";
    func_expect << "struct A_StructB_Mapa_submap2 Init_0_A_StructB_Mapa_submap2();";
    func_expect << "struct A_StructB_Mapa_submap2 ND_A_StructB_Mapa_submap2();";
    func_expect << "unsigned int Read_A_StructB_Mapa_submap2"
                << "(struct A_StructB_Mapa_submap2*a,unsigned int idx);";
    func_expect << "void Write_A_StructB_Mapa_submap2(struct A_StructB_Mapa_submap2*a,"
                << "unsigned int idx,unsigned int d);";
    func_expect << "unsigned int*Ref_A_StructB_Mapa_submap2"
                << "(struct A_StructB_Mapa_submap2*a,unsigned int idx);";

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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB_Mapa_submap1;";
    adt_expect << "struct A_StructB;";
    adt_expect << "struct A;";
    adt_expect << "struct C_Mapb_submap1;";
    adt_expect << "struct C;";
    func_expect << "struct A Init_A();";
    func_expect << "struct A_StructB Init_0_A_StructB();";
    func_expect << "struct A_StructB Init_A_StructB();";
    func_expect << "struct A_StructB ND_A_StructB();";
    func_expect << "struct A_StructB_Mapa_submap1 Init_0_A_StructB_Mapa_submap1();";
    func_expect << "struct A_StructB_Mapa_submap1 ND_A_StructB_Mapa_submap1();";
    func_expect << "unsigned int Read_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*a,unsigned int idx);";
    func_expect << "void Write_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*a,unsigned int idx,unsigned int d);";
    func_expect << "unsigned int*Ref_A_StructB_Mapa_submap1"
                << "(struct A_StructB_Mapa_submap1*a,unsigned int idx);";
    func_expect << "struct C Init_C();";
    func_expect << "struct C_Mapb_submap1 Init_0_C_Mapb_submap1();";
    func_expect << "struct C_Mapb_submap1 ND_C_Mapb_submap1();";
    func_expect << "unsigned int Read_C_Mapb_submap1"
                << "(struct C_Mapb_submap1*a,unsigned int idx);";
    func_expect << "void Write_C_Mapb_submap1"
                << "(struct C_Mapb_submap1*a,unsigned int idx,unsigned int d);";
    func_expect << "unsigned int*Ref_C_Mapb_submap1"
                << "(struct C_Mapb_submap1*a,unsigned int idx);";

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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_Mapa_submap3;";
    adt_expect << "struct A_Mapa_submap2;";
    adt_expect << "struct A_Mapa_submap1;";
    adt_expect << "struct A;";
    func_expect << "struct A Init_A();";
    func_expect << "struct A_Mapa_submap1 Init_0_A_Mapa_submap1();";
    func_expect << "struct A_Mapa_submap1 ND_A_Mapa_submap1();";
    func_expect << "struct A_Mapa_submap2 Read_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*a,unsigned int idx);";
    func_expect << "void Write_A_Mapa_submap1(struct A_Mapa_submap1*a"
                << ",unsigned int idx,struct A_Mapa_submap2 d);";
    func_expect << "struct A_Mapa_submap2*Ref_A_Mapa_submap1"
                << "(struct A_Mapa_submap1*a,unsigned int idx);";
    func_expect << "struct A_Mapa_submap2 Init_0_A_Mapa_submap2();";
    func_expect << "struct A_Mapa_submap2 ND_A_Mapa_submap2();";
    func_expect << "struct A_Mapa_submap3 Read_A_Mapa_submap2"
                << "(struct A_Mapa_submap2*a,unsigned int idx);";
    func_expect << "void Write_A_Mapa_submap2(struct A_Mapa_submap2*a"
                << ",unsigned int idx,struct A_Mapa_submap3 d);";
    func_expect << "struct A_Mapa_submap3*Ref_A_Mapa_submap2"
                << "(struct A_Mapa_submap2*a,unsigned int idx);";
    func_expect << "struct A_Mapa_submap3 Init_0_A_Mapa_submap3();";
    func_expect << "struct A_Mapa_submap3 ND_A_Mapa_submap3();";
    func_expect << "unsigned int Read_A_Mapa_submap3"
                << "(struct A_Mapa_submap3*a,unsigned int idx);";
    func_expect << "void Write_A_Mapa_submap3"
                << "(struct A_Mapa_submap3*a,unsigned int idx,unsigned int d);";
    func_expect << "unsigned int*Ref_A_Mapa_submap3"
                << "(struct A_Mapa_submap3*a,unsigned int idx);";

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
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, true
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A_StructB;" << "struct A;";
    func_expect << "struct A Init_A();";
    func_expect << "struct A_StructB Init_0_A_StructB();";
    func_expect << "struct A_StructB Init_A_StructB(unsigned int a);";
    func_expect << "struct A_StructB ND_A_StructB();";
    func_expect << "struct A_StructB Method_A_FuncadvFunc"
                << "(struct A*self,struct CallState*state,unsigned int _in);";

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    ostringstream adt_actual, func_actual;
    ADTConverter(ast, converter, false).print(adt_actual);
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, false
    ).print(func_actual);

    ostringstream adt_expect, func_expect;
    // -- A_StructS
    adt_expect << "struct A_StructS";
    adt_expect << "{";
    adt_expect << "int d_owner;";
    adt_expect << "unsigned int d_val;";
    adt_expect << "};";
    // -- A_Mapaccs_submap1
    adt_expect << "struct A_Mapaccs_submap1";
    adt_expect << "{";
    adt_expect << "int m_set;";
    adt_expect << "unsigned int m_curr;";
    adt_expect << "struct A_StructS d_;";
    adt_expect << "struct A_StructS d_nd;";
    adt_expect << "};";
    // -- A
    adt_expect << "struct A";
    adt_expect << "{";
    adt_expect << "unsigned int d_min_amt;";
    adt_expect << "struct A_Mapaccs_submap1 d_accs;";
    adt_expect << "};";
    // -- Init_A
    func_expect << "struct A Init_A()";
    func_expect << "{";
    func_expect << "struct A tmp;";
    func_expect << "((tmp).d_min_amt)=(42);";
    func_expect << "((tmp).d_accs)=(Init_0_A_Mapaccs_submap1());";
    func_expect << "return tmp;";
    func_expect << "}";
    // -- Init_0_A_StructS
    func_expect << "struct A_StructS Init_0_A_StructS()";
    func_expect << "{";
    func_expect << "struct A_StructS tmp;";
    func_expect << "((tmp).d_owner)=(0);";
    func_expect << "((tmp).d_val)=(0);";
    func_expect << "return tmp;";
    func_expect << "}";
    // -- Init_A_StructS
    func_expect << "struct A_StructS Init_A_StructS(int owner,unsigned int val)";
    func_expect << "{";
    func_expect << "struct A_StructS tmp=Init_0_A_StructS();";
    func_expect << "((tmp).d_owner)=(owner);";
    func_expect << "((tmp).d_val)=(val);";
    func_expect << "return tmp;";
    func_expect << "}";
    // -- ND_A_StructS
    func_expect << "struct A_StructS ND_A_StructS()";
    func_expect << "{";
    func_expect << "struct A_StructS tmp;";
    func_expect << "((tmp).d_owner)=(ND_Init_Val());";
    func_expect << "((tmp).d_val)=(ND_Init_Val());";
    func_expect << "return tmp;";
    func_expect << "}";
    // -- Init_A_Mapaccs_submap1
    func_expect << "struct A_Mapaccs_submap1 Init_0_A_Mapaccs_submap1()";
    func_expect << "{";
    func_expect << "struct A_Mapaccs_submap1 tmp;";
    func_expect << "((tmp).m_set)=(0);";
    func_expect << "((tmp).m_curr)=(0);";
    func_expect << "((tmp).d_)=(Init_0_A_StructS());";
    func_expect << "((tmp).d_nd)=(Init_0_A_StructS());";
    func_expect << "return tmp;";
    func_expect << "}";
    // -- ND_A_Mapaccs_submap1
    func_expect << "struct A_Mapaccs_submap1 ND_A_Mapaccs_submap1()";
    func_expect << "{";
    func_expect << "struct A_Mapaccs_submap1 tmp;";
    func_expect << "((tmp).m_set)=(ND_Init_Val());";
    func_expect << "((tmp).m_curr)=(ND_Init_Val());";
    func_expect << "((tmp).d_)=(ND_A_StructS());";
    func_expect << "((tmp).d_nd)=(Init_0_A_StructS());";
    func_expect << "return tmp;";
    func_expect << "}";
    // -- Read_A_Mapaccs_submap1
    func_expect << "struct A_StructS Read_A_Mapaccs_submap1"
                << "(struct A_Mapaccs_submap1*a,unsigned int idx)";
    func_expect << "{";
    func_expect << "if(((a)->m_set)==(0)){((a)->m_curr)=(idx);((a)->m_set)=(1);}";
    func_expect << "if((idx)!=((a)->m_curr))return ND_A_StructS();";
    func_expect << "return (a)->d_;";
    func_expect << "}";
    // -- Write_A_Mapaccs_submap1
    func_expect << "void Write_A_Mapaccs_submap1"
                << "(struct A_Mapaccs_submap1*a,unsigned int idx,struct A_StructS d)";
    func_expect << "{";
    func_expect << "if(((a)->m_set)==(0)){((a)->m_curr)=(idx);((a)->m_set)=(1);}";
    func_expect << "if((idx)==((a)->m_curr))((a)->d_)=(d);";
    func_expect << "}";
    // -- Ref_A_Mapaccs_submap1
    func_expect << "struct A_StructS*Ref_A_Mapaccs_submap1"
                << "(struct A_Mapaccs_submap1*a,unsigned int idx)";
    func_expect << "{";
    func_expect << "if(((a)->m_set)==(0)){((a)->m_curr)=(idx);((a)->m_set)=(1);}";
    func_expect << "if((idx)!=((a)->m_curr))";
    func_expect << "{";
    func_expect << "((a)->d_nd)=(ND_A_StructS());";
    func_expect << "return &((a)->d_nd);";
    func_expect << "}";
    func_expect << "return &((a)->d_);";
    func_expect << "}";
    // -- Method_A_Open
    func_expect << "void Method_A_FuncOpen"
                << "(struct A*self,struct CallState*state,unsigned int idx)";
    func_expect << "{";
    func_expect << "assume(((Read_A_Mapaccs_submap1(&(self->d_accs),idx)).d_owner"
                << ")==(((int)(0))));";
    func_expect << "Write_A_Mapaccs_submap1(&(self->d_accs),idx"
                << ",Init_A_StructS(state->sender,0));";
    func_expect << "}";
    // -- Method_A_Deposit
    func_expect << "void Method_A_FuncDeposit"
                << "(struct A*self,struct CallState*state,unsigned int idx)";
    func_expect << "{";
    func_expect << "assume((state->value)>(self->d_min_amt));";
    func_expect << "struct A_StructS*entry=Ref_A_Mapaccs_submap1(&(self->d_accs),idx);";
    func_expect << "if(((entry)->d_owner)!=(state->sender))";
    func_expect << "{";
    func_expect << "Method_A_FuncOpen(self,state,idx);";
    func_expect << "}";
    func_expect << "((entry)->d_val)=(((entry)->d_val)+(state->value));";
    func_expect << "}";
    // -- Method_A_Withdraw
    func_expect << "void Method_A_FuncWithdraw"
                << "(struct A*self,struct CallState*state,unsigned int idx)";
    func_expect << "{";
    func_expect << "assume(((Read_A_Mapaccs_submap1(&(self->d_accs),idx)).d_owner"
                << ")==(state->sender));";
    func_expect << "unsigned int amt="
                << "(Read_A_Mapaccs_submap1(&(self->d_accs),idx)).d_val;";
    func_expect << "Write_A_Mapaccs_submap1(&(self->d_accs),idx"
                << ",Init_A_StructS(state->sender,0));";
    func_expect << "assert(((Read_A_Mapaccs_submap1(&(self->d_accs),idx)).d_val"
                << ")==(0));";
    func_expect << "_pay(state,state->sender,amt);";
    func_expect << "}";
    // -- Method_A_View
    func_expect << "unsigned int Method_A_FuncView"
                << "(struct A*self,struct CallState*state,unsigned int idx)";
    func_expect << "{";
    func_expect << "unsigned int amt;";
    func_expect << "(amt)=((Read_A_Mapaccs_submap1(&(self->d_accs),idx)).d_val);";
    func_expect << "return amt;";
    func_expect << "}";

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
    ADTConverter(ast, converter, false).print(adt_1);
    ADTConverter(ast, converter, false).print(adt_2);
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, false
    ).print(func_1);
    FunctionConverter(
        ast, converter, FunctionConverter::View::FULL, false
    ).print(func_2);

    BOOST_CHECK_EQUAL(adt_1.str(), adt_2.str());
    BOOST_CHECK_EQUAL(func_1.str(), func_2.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
