/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/BlockConversionVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/BlockConverter.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <sstream>

using namespace std;
using langutil::SourceLocation;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(
    BlockConversion,
    ::dev::solidity::test::AnalysisFramework
)

// Tests that input parameters are registered as declarations.
BOOST_AUTO_TEST_CASE(argument_registration)
{
    char const* text = R"(
		contract A {
			function f(int a, int b) public {
                a;
                b;
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual;
    actual << *BlockConverter(func, converter).convert();
    BOOST_CHECK_EQUAL(actual.str(), "{a;b;}");
}

// Tests that else statements and bodies are optional and that branch bodies are
// properly scoped
BOOST_AUTO_TEST_CASE(if_statement)
{
    char const* text = R"(
		contract A {
            int a;
			function if_stmt() public {
                if (a == 1) { }
                if (a == 1) { int a; }
                a;
            }
			function if_else_stmt() public {
                if (a == 1) { }
                else { }
                if (a == 1) { int a; }
                else { int a; }
                a;
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    auto if_stmt = (fncs[0]->name() == "if_stmt") ? fncs[0] : fncs[1];
    auto else_stmt = (fncs[0]->name() == "if_stmt") ? fncs[1] : fncs[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual_if, expected_if;
    actual_if << *BlockConverter(*if_stmt, converter).convert();
    expected_if << "{";
    expected_if << "if((self->d_a)==(1))";
    expected_if << "{";
    expected_if << "}";
    expected_if << "if((self->d_a)==(1))";
    expected_if << "{";
    expected_if << "int256_t a;";
    expected_if << "}";
    expected_if << "self->d_a;";
    expected_if << "}";
    BOOST_CHECK_EQUAL(actual_if.str(), expected_if.str());

    ostringstream actual_else, expected_else;
    actual_else << *BlockConverter(*else_stmt, converter).convert();
    expected_else << "{";
    expected_else << "if((self->d_a)==(1)){}";
    expected_else << "else {}";
    expected_else << "if((self->d_a)==(1)){int256_t a;}";
    expected_else << "else {int256_t a;}";
    expected_else << "self->d_a;";
    expected_else << "}";
    BOOST_CHECK_EQUAL(actual_else.str(), expected_else.str());
}

// Tests that while and for loops work in general, that expressions of a for
// loop are optional, and that loops are correctly scoped.
BOOST_AUTO_TEST_CASE(loop_statement)
{
    char const* text = R"(
		contract A {
            uint a;
            uint i;
			function while_stmt() public {
                while (a != a) { }
                while (a != a) { int i; }
                i;
            }
            function for_stmt() public {
                for (; a < 10; ++a) { int i; }
                for (int i = 0; ; ++i) { i; }
                for (int i = 0; i < 10; ) { ++i; }
                for (int i = 0; i < 10; ++i) { }
                i;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    TypeConverter converter;
    converter.record(unit);

    auto while_stmt = (fncs[0]->name() == "while_stmt") ? fncs[0] : fncs[1];
    ostringstream actual_while, expected_while;
    actual_while << *BlockConverter(*while_stmt, converter).convert();
    expected_while << "{";
    expected_while << "while((self->d_a)!=(self->d_a)){}";
    expected_while << "while((self->d_a)!=(self->d_a)){int256_t i;}";
    expected_while << "self->d_i;";
    expected_while << "}";
    BOOST_CHECK_EQUAL(actual_while.str(), expected_while.str());

    auto for_stmt = (fncs[0]->name() == "while_stmt") ? fncs[1] : fncs[0];
    ostringstream actual_for, expected_for;
    actual_for << *BlockConverter(*for_stmt, converter).convert();
    expected_for << "{";
    expected_for << "for(;(self->d_a)<(10);++(self->d_a)){int256_t i;}";
    expected_for << "for(int256_t i=0;;++(i)){i;}";
    expected_for << "for(int256_t i=0;(i)<(10);){++(i);}";
    expected_for << "for(int256_t i=0;(i)<(10);++(i)){}";
    expected_for << "self->d_i;";
    expected_for << "}";
    BOOST_CHECK_EQUAL(actual_for.str(), expected_for.str());
}

// Ensures continue statements remain unchanged.
BOOST_AUTO_TEST_CASE(continue_statement)
{
    char const* text = R"(
		contract A {
			function void_func() public {
                while (false) { continue; }
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual;
    actual << *BlockConverter(func, converter).convert();
    BOOST_CHECK_EQUAL(actual.str(), "{while(0){continue;}}");
}

// Ensures break statements remain unchanged.
BOOST_AUTO_TEST_CASE(break_statement)
{
    char const* text = R"(
		contract A {
			function void_func() public {
                while (false) { break; }
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual;
    actual << *BlockConverter(func, converter).convert();
    BOOST_CHECK_EQUAL(actual.str(), "{while(0){break;}}");
}

// Ensures return statements remain unchanged.
BOOST_AUTO_TEST_CASE(return_statement)
{
    char const* text = R"(
		contract A {
			function void_func() public { return; }
            function int_func() public returns (int) { return 10 + 5; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    TypeConverter converter;
    converter.record(unit);

    auto void_func = (fncs[0]->name() == "void_func") ? fncs[0] : fncs[1];
    ostringstream actual_void;
    actual_void << *BlockConverter(*void_func, converter).convert();
    BOOST_CHECK_EQUAL(actual_void.str(), "{return;}");

    auto int_func = (fncs[0]->name() == "void_func") ? fncs[1] : fncs[0];
    ostringstream actual_int;
    actual_int << *BlockConverter(*int_func, converter).convert();
    BOOST_CHECK_EQUAL(actual_int.str(), "{return (10)+(5);}");
}

// Ensures that variable declarations will generate C declarations, and that
// these declarations will be added to the internal state of the block
// converter. Also ensures that these variables will be popped once said scope
// is exited.
BOOST_AUTO_TEST_CASE(variable_declaration_statement)
{
    char const* text = R"(
		contract A {
            int a;
            int c;
			function f() public {
                int b;
                {
                    int c;
                    a; b; c;
                }
                { a; b; c; }
                a; b; c;
            }
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "int256_t b;";
    expected << "{";
    expected << "int256_t c;";
    expected << "self->d_a;";
    expected << "b;";
    expected << "c;";
    expected << "}";
    expected << "{";
    expected << "self->d_a;";
    expected << "b;";
    expected << "self->d_c;";
    expected << "}";
    expected << "self->d_a;";
    expected << "b;";
    expected << "self->d_c;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that if a function has named return values, that those variables will
// be implicitly declared.
BOOST_AUTO_TEST_CASE(named_function_retvars)
{
    char const* text = R"(
		contract A {
			function f() public returns (int) { return 5; }
			function g() public returns (int a) { a = 5; }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& fncs = ctrt.definedFunctions();

    TypeConverter converter;
    converter.record(unit);

    auto unnamed = (fncs[0]->name() == "f") ? fncs[0] : fncs[1];
    ostringstream actual_unnamed;
    actual_unnamed << *BlockConverter(*unnamed, converter).convert();
    BOOST_CHECK_EQUAL(actual_unnamed.str(), "{return 5;}");

    auto named = (fncs[0]->name() == "f") ? fncs[1] : fncs[0];
    ostringstream actual_named, expected_named;
    actual_named << *BlockConverter(*named, converter).convert();
    expected_named << "{";
    expected_named << "int256_t a;";
    expected_named << "(a)=(5);";
    expected_named << "return a;";
    expected_named << "}";
    BOOST_CHECK_EQUAL(actual_named.str(), expected_named.str());
}

// Tests type-aware resolution of MemberAccess expressions. This is tested on
// the block level as type annotation is not trial, and automatically handled by
// the analysis framework.
BOOST_AUTO_TEST_CASE(member_access_expressions)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            struct C { B b; }
            B b;
            C c;
            int public d;
            function f() public payable {
                this.d;
                b.i;
                c.b.i;
                block.number;
                block.timestamp;
                msg.sender;
                msg.value;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto &func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "(self)->d_d;";
    expected << "(self->d_b).d_i;";
    expected << "((self->d_c).d_b).d_i;";
    expected << "state->blocknum;";
    expected << "state->blocknum;";
    expected << "state->sender;";
    expected << "state->value;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests interal method calls. Internal calls are those which reference contract
// members, without the use of member access (`<ctx>.<method>(...)` or
// `this.<method>(...)`).
BOOST_AUTO_TEST_CASE(internal_method_calls)
{
    char const* text = R"(
		contract A {
			function f() public { }
            function g(int a) public { }
            function h(int a, int b) public { }
            function p() public pure { }
            function q(int a) public pure { }
            function r(int a, int b) public pure { }
            function test() public {
                f();
                g(1);
                h(1, 2);
                p();
                q(1);
                r(1, 2);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");

    TypeConverter converter;
    converter.record(unit);
    
    for (auto func_ptr : ctrt.definedFunctions())
    {
        if (func_ptr->name() == "test")
        {
            ostringstream actual, expected;
            actual << *BlockConverter(*func_ptr, converter).convert();
            expected << "{";
            expected << "Method_A_Funcf(self,state);";
            expected << "Method_A_Funcg(self,state,1);";
            expected << "Method_A_Funch(self,state,1,2);";
            expected << "Method_A_Funcp();";
            expected << "Method_A_Funcq(1);";
            expected << "Method_A_Funcr(1,2);";
            expected << "}";
            BOOST_CHECK_EQUAL(actual.str(), expected.str());
            break;
        }
    }
}

// Tests external method calls. External method calls reference members of a
// given contract by address, through the use of member accessors
// (`<ctx>.<method>(...)` or `this.<method>(...)`).
BOOST_AUTO_TEST_CASE(external_method_calls)
{
    char const* text = R"(
		contract A {
			function f() public { }
            function g() public pure { }
		}
        contract B {
            A a;
            B b;
            function f() public { }
            function test() public {
                a.f();
                a.g();
                b.f();
                this.f();
                (this.f)();
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "B");

    TypeConverter converter;
    converter.record(unit);
    
    for (auto func_ptr : ctrt.definedFunctions())
    {
        if (func_ptr->name() == "test")
        {
            ostringstream actual, expected;
            actual << *BlockConverter(*func_ptr, converter).convert();
            expected << "{";
            expected << "Method_A_Funcf(&(self->d_a),state);";
            expected << "Method_A_Funcg();";
            expected << "Method_B_Funcf(&(self->d_b),state);";
            expected << "Method_B_Funcf(self,state);";
            expected << "Method_B_Funcf(self,state);";
            expected << "}";;
            BOOST_CHECK_EQUAL(actual.str(), expected.str());
            break;
        }
    }
}

// Tests conversion of transfer/send into _pay. This is tested on the block
// level due to the complexity of annotated FunctionCall expressions.
BOOST_AUTO_TEST_CASE(payment_function_calls)
{
    char const* text = R"(
		contract A {
			function f(address payable dst) public {
                dst.transfer(5);
                dst.send(10);
                (dst.send)(15);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "_pay(state,dst,5);";
    expected << "_pay(state,dst,10);";
    expected << "_pay(state,dst,15);";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests conversion of assert/require into C-horn. This is tested on the block
// level due to the complexity of annotated FunctionCall expressions.
BOOST_AUTO_TEST_CASE(verification_function_calls)
{
    char const* text = R"(
		contract A {
			function f(address payable dst) public {
                require(true);
                require(true, "test");
                assert(true);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "assume(1);";
    expected << "assume(1);";
    expected << "assert(1);";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that `<struct>(<v1>, ..., <vn>)` will be mapped to
// `Init_<contract>_<struct>(<v1>, ..., <vn>)`.
BOOST_AUTO_TEST_CASE(struct_ctor_calls)
{
    char const* text = R"(
		contract A {
            struct B { mapping(uint => uint) a; }
            struct C { uint a; }
            struct D { uint a; uint b; }
			function f() public {
                B(); C(1); D(1, 2);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "Init_A_StructB();";
    expected << "Init_A_StructC(1);";
    expected << "Init_A_StructD(1,2);";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that `new <contract>(<v1>, ..., <vn>)` will be mapped to
// `Init_<contract>(<v1>, ..., <vn>)`.
BOOST_AUTO_TEST_CASE(contract_ctor_calls)
{
    char const* text = R"(
        contract A { }
        contract B {
            int a;
            constructor(int _a) public {
                a = _a;
            }
        }
		contract C {
			function f() public {
                new A();
                new B(10);
            }
		}
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "C");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "Init_A();";
    expected << "Init_B(nullptr,state,10);";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensure that when not caught by another expression (ie, assignmet), that an
// index access to a map will be replaced by a Read_ call. Furthermore, this
// ensures that nested index accesses will be replaced by Ref_ calls.
BOOST_AUTO_TEST_CASE(read_only_index_access)
{
    char const* text = R"(
        contract A {
            struct B { mapping(int => mapping(int => int)) arr2; }
            struct C { B b; }
            mapping(int => mapping(int => int)) arr1;
            B b;
            C c;
            function f() public {
                arr1[1 + 2];
                b.arr2[3 + 4];
                c.b.arr2[5 + 6];
                arr1[10][10];
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{"
             << "Read_A_Maparr1_submap1(&(self->d_arr1),(1)+(2));"
             << "Read_A_StructB_Maparr2_submap1(&((self->d_b).d_arr2),(3)+(4));"
             << "Read_A_StructB_Maparr2_submap1(&(((self->d_c).d_b).d_arr2),(5)+(6));"
             << "Read_A_Maparr1_submap2(Ref_A_Maparr1_submap1(&(self->d_arr1),10),10);"
             << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that when the l-value of an assignment is an IndexAccess, that the
// forementioned assignment will be rewritten as a Write_ call.
BOOST_AUTO_TEST_CASE(map_assignment)
{
    char const* text = R"(
        contract A {
            struct B { int m; }
            struct C { mapping(int => int) m; }
            mapping(int => int) a;
            mapping(int => B) b;
            C c;
            mapping(int => mapping(int => int)) d;
            function f() public {
                a[1] = 2;
                a[1] += 2;
                b[1].m += 2;
                c.m[1] = 2;
                d[1][2] = 3;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "Write_A_Mapa_submap1(&(self->d_a),1,2);";
    expected << "Write_A_Mapa_submap1(&(self->d_a),1"
             << ",(Read_A_Mapa_submap1(&(self->d_a),1))+(2));";
    expected << "((*(Ref_A_Mapb_submap1(&(self->d_b),1))).d_m)=("
             << "((Read_A_Mapb_submap1(&(self->d_b),1)).d_m)+(2));";
    expected << "Write_A_StructC_Mapm_submap1(&((self->d_c).d_m),1,2);";
    expected << "Write_A_Mapd_submap2(Ref_A_Mapd_submap1(&(self->d_d),1),2,3);";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests all supported typecasts in their most explicit forms.
BOOST_AUTO_TEST_CASE(type_casting)
{
    char const* text = R"(
        contract A {
            address a;
            int s;
            uint u;
            bool b;
            function f() public view {
                address(5.0);
                address(a); int(a); uint(a);
                address(s); int(s); uint(s);
                address(u); int(u); uint(u);
                bool(b);
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "((int)(5));";
    expected << "self->d_a;";
    expected << "self->d_a;";
    expected << "((unsigned int)(self->d_a));";
    expected << "self->d_s;";
    expected << "self->d_s;";
    expected << "((unsigned int)(self->d_s));";
    expected << "((int)(self->d_u));";
    expected << "((int)(self->d_u));";
    expected << "self->d_u;";
    expected << "self->d_b;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests that storage variables may be declared within a function, and that they
// are deferencable.
BOOST_AUTO_TEST_CASE(storage_variable_resolution)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            B b;
            function f() public view {
                B storage b_ref = b;
                b_ref.i;
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "struct A_StructB*b_ref=&(self->d_b);";
    expected << "(b_ref)->d_i;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests that storage variables may be assigned to from storage. A reference to
// said storage must be acquired.
BOOST_AUTO_TEST_CASE(storage_variable_assignment)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            B b;
            function f() public view {
                B storage b_ref = b;
                b_ref = b;
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "struct A_StructB*b_ref=&(self->d_b);";
    expected << "(b_ref)=(&(self->d_b));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests that storage variables may be assigned to from a mapping. A reference
// to said map must be acquired.
BOOST_AUTO_TEST_CASE(storage_variable_to_map)
{
    char const* text = R"(
        contract A {
            struct B { int i; }
            mapping(int => B) a;
            function f() public view {
                B storage b_ref = a[0];
                b_ref = a[0];
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "struct A_StructB*b_ref=Ref_A_Mapa_submap1(&(self->d_a),0);";
    expected << "(b_ref)=(Ref_A_Mapa_submap1(&(self->d_a),0));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Regression test to ensure "else if" is not contracted into "elseif".
BOOST_AUTO_TEST_CASE(else_if_formatting_regression)
{
    char const* text = R"(
        contract A {
            function f() public view {
                if (true) {} else if (false) {}
            }
        }
	)";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(unit);

    ostringstream actual, expected;
    actual << *BlockConverter(func, converter).convert();
    expected << "{";
    expected << "if(1){}";
    expected << "else if(0){}";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
