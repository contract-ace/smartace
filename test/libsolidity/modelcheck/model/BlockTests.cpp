/**
 * Specific tests for libsolidity/modelcheck/model/Block.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/model/Block.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/utils/Function.h>

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

// -------------------------------------------------------------------------- //

BOOST_FIXTURE_TEST_SUITE(
    Model_BlockTests, ::dev::solidity::test::AnalysisFramework
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expect;
    actual << *FunctionBlockConverter(func, stack).convert();
    expect << "{";
    expect << "(func_user_a).v;";
    expect << "(func_user_b).v;";
    expect << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    auto if_stmt = ctrt->definedFunctions()[0];
    auto else_stmt = ctrt->definedFunctions()[1];

    BOOST_CHECK_EQUAL(if_stmt->name(), "if_stmt");
    BOOST_CHECK_EQUAL(else_stmt->name(), "if_else_stmt");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual_if, expected_if;
    actual_if << *FunctionBlockConverter(*if_stmt, stack).convert();
    expected_if << "{";
    expected_if << "if(((self->user_a).v)==(1))";
    expected_if << "{";
    expected_if << "}";
    expected_if << "if(((self->user_a).v)==(1))";
    expected_if << "{";
    expected_if << "sol_int256_t func_user_a;";
    expected_if << "}";
    expected_if << "(self->user_a).v;";
    expected_if << "}";
    BOOST_CHECK_EQUAL(actual_if.str(), expected_if.str());

    ostringstream actual_else, expected_else;
    actual_else << *FunctionBlockConverter(*else_stmt, stack).convert();
    expected_else << "{";
    expected_else << "if(((self->user_a).v)==(1)){}";
    expected_else << "else {}";
    expected_else << "if(((self->user_a).v)==(1)){sol_int256_t func_user_a;}";
    expected_else << "else {sol_int256_t func_user_a;}";
    expected_else << "(self->user_a).v;";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    auto while_stmt = ctrt->definedFunctions()[0];
    auto for_stmt = ctrt->definedFunctions()[1];

    BOOST_CHECK_EQUAL(while_stmt->name(), "while_stmt");
    BOOST_CHECK_EQUAL(for_stmt->name(), "for_stmt");

    ostringstream actual_while, expected_while;
    actual_while << *FunctionBlockConverter(*while_stmt, stack).convert();
    expected_while << "{";
    expected_while << "while(((self->user_a).v)!=((self->user_a).v)){}";
    expected_while << "while(((self->user_a).v)!=((self->user_a).v))";
    expected_while << "{sol_int256_t func_user_i;}";
    expected_while << "(self->user_i).v;";
    expected_while << "}";
    BOOST_CHECK_EQUAL(actual_while.str(), expected_while.str());

    ostringstream actual_for, expected_for;
    actual_for << *FunctionBlockConverter(*for_stmt, stack).convert();
    expected_for << "{";
    expected_for << "for(;((self->user_a).v)<(10);++((self->user_a).v))"
                 << "{sol_int256_t func_user_i;}";
    expected_for << "for(sol_int256_t func_user_i=Init_sol_int256_t(0);;++("
                 << "(func_user_i).v)){(func_user_i).v;}";
    expected_for << "for(sol_int256_t func_user_i=Init_sol_int256_t(0);"
                 << "((func_user_i).v)<(10);)"
                 << "{++((func_user_i).v);}";
    expected_for << "for(sol_int256_t func_user_i=Init_sol_int256_t(0);"
                 << "((func_user_i).v)<(10);++((func_user_i).v)){}";
    expected_for << "(self->user_i).v;";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expect;
    actual << *FunctionBlockConverter(func, stack).convert();
    expect << "{";
    expect << "while(0){continue;}";
    expect << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expect;
    actual << *FunctionBlockConverter(func, stack).convert();
    expect << "{";
    expect << "while(0){break;}";
    expect << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    auto void_func = ctrt->definedFunctions()[0];
    auto int_func = ctrt->definedFunctions()[1];

    BOOST_CHECK_EQUAL(void_func->name(), "void_func");
    BOOST_CHECK_EQUAL(int_func->name(), "int_func");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual_void, expect_void;
    actual_void << *FunctionBlockConverter(*void_func, stack).convert();
    expect_void << "{";
    expect_void << "return;";
    expect_void << "}";
    BOOST_CHECK_EQUAL(actual_void.str(), expect_void.str());

    ostringstream actual_int, expect_int;
    actual_int << *FunctionBlockConverter(*int_func, stack).convert();
    expect_int << "{";
    expect_int << "return Init_sol_int256_t((10)+(5));";
    expect_int << "}";
    BOOST_CHECK_EQUAL(actual_int.str(), expect_int.str());
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

    auto const &unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const &func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "sol_int256_t func_user_b;";
    expected << "{";
    expected << "sol_int256_t func_user_c;";
    expected << "(self->user_a).v;";
    expected << "(func_user_b).v;";
    expected << "(func_user_c).v;";
    expected << "}";
    expected << "{";
    expected << "(self->user_a).v;";
    expected << "(func_user_b).v;";
    expected << "(self->user_c).v;";
    expected << "}";
    expected << "(self->user_a).v;";
    expected << "(func_user_b).v;";
    expected << "(self->user_c).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that if a function has named return values, that those variables will
// be implicitly declared.
BOOST_AUTO_TEST_CASE(named_function_retvars)
{
    char const* text = R"(
		contract A {
			function f() public returns (int a) { a = 5; }
		}
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual_named, expected_named;
    actual_named << *FunctionBlockConverter(func, stack).convert();
    expected_named << "{";
    expected_named << "sol_int256_t func_user_a=Init_sol_int256_t(0);";
    expected_named << "((func_user_a).v)=(5);";
    expected_named << "return func_user_a;";
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
                address(this).balance;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "if(((paid).v)==(1))(((self)->model_balance).v)+=((value).v);";
    expected << "(self)->user_d;";
    expected << "((self->user_b).user_i).v;";
    expected << "(((self->user_c).user_b).user_i).v;";
    expected << "(blocknum).v;";
    expected << "(timestamp).v;";
    expected << "(sender).v;";
    expected << "(value).v;";
    expected << "((self)->model_balance).v;";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    auto func = ctrt->definedFunctions()[6];
    BOOST_CHECK_EQUAL(func->name(), "test");

    ostringstream actual, expected;
    FunctionBlockConverter fbc(*func, stack);
    fbc.set_for(FunctionSpecialization(*func));
    actual << *fbc.convert();
    expected << "{";
    expected << "A_Method_f(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin);";
    expected << "A_Method_g(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin,Init_sol_int256_t(1));";
    expected << "A_Method_h(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin,Init_sol_int256_t(1)"
             << ",Init_sol_int256_t(2));";
    expected << "A_Method_p(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin);";
    expected << "A_Method_q(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin,Init_sol_int256_t(1));";
    expected << "A_Method_r(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin,Init_sol_int256_t(1)"
             << ",Init_sol_int256_t(2));";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
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
            constructor() public {
                a = new A();
            }
            function f() public { }
            function test() public {
                a.f();
                a.g();
                this.f();
                (this.f)();
            }
        }
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "B");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    auto func = ctrt->definedFunctions()[2];
    BOOST_CHECK_EQUAL(func->name(), "test");

    ostringstream actual, expected;
    FunctionBlockConverter fbc(*func, stack);
    fbc.set_for(FunctionSpecialization(*func));
    actual << *fbc.convert();
    expected << "{";
    expected << "A_Method_f(&(self->user_a),(self)->model_address"
             << ",Init_sol_uint256_t(0),blocknum,timestamp,Init_sol_bool_t(1)"
             << ",origin);";
    expected << "A_Method_g(&(self->user_a),(self)->model_address"
             << ",Init_sol_uint256_t(0),blocknum,timestamp,Init_sol_bool_t(1)"
             << ",origin);";
    expected << "B_Method_f(self,(self)->model_address,Init_sol_uint256_t(0)"
             << ",blocknum,timestamp,Init_sol_bool_t(1),origin);";
    expected << "B_Method_f(self,(self)->model_address,Init_sol_uint256_t(0)"
             << ",blocknum,timestamp,Init_sol_bool_t(1),origin);";
    expected << "}";;
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Tests conversion of transfer/send into _pay. This is tested on the block
// level due to the complexity of annotated FunctionCall expressions.
BOOST_AUTO_TEST_CASE(payment_to_addr_calls)
{
    char const* text = R"(
		contract A {
			function f(address payable dst) public {
                dst.transfer(5);
                dst.send(10);
                (dst.send)(15);
                address(20).send(25);
            }
		}
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "sol_transfer(&((self)->model_balance),Init_sol_address_t("
             << "(func_user_dst).v),Init_sol_uint256_t(5));";
    expected << "sol_send(&((self)->model_balance),Init_sol_address_t("
             << "(func_user_dst).v),Init_sol_uint256_t(10));";
    expected << "sol_send(&((self)->model_balance),Init_sol_address_t("
             << "(func_user_dst).v),Init_sol_uint256_t(15));";
    expected << "sol_send(&((self)->model_balance),Init_sol_address_t(((int)("
             << "g_literal_address_20))),Init_sol_uint256_t(25));";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "sol_require(1,0);";
    expected << "sol_require(1,\"test\");";
    expected << "sol_assert(1,0);";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "Init_A_Struct_B();";
    expected << "Init_A_Struct_C(Init_sol_uint256_t(1));";
    expected << "Init_A_Struct_D(Init_sol_uint256_t(1),Init_sol_uint256_t(2));";
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
            A a;
            B b;
			constructor() public {
                a = new A();
                b = new B(10);
            }
		}
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "C");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "Init_A(&(self->user_a),(self)->model_address"
             << ",Init_sol_uint256_t(0),blocknum,timestamp,Init_sol_bool_t(1)"
             << ",origin);";
    expected << "Init_B(&(self->user_b),(self)->model_address"
             << ",Init_sol_uint256_t(0),blocknum,timestamp,Init_sol_bool_t(1)"
             << ",origin,Init_sol_int256_t(10));";
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
            struct B { mapping(address => mapping(address => int)) arr2; }
            struct C { B b; }
            mapping(address => mapping(address => int)) arr1;
            B b;
            C c;
            function f(address i) public {
                b.arr2[i][i];
                c.b.arr2[i][i];
                arr1[i][i];
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "(Read_Map_1(&((self->user_b).user_arr2),"
             << "Init_sol_address_t((func_user_i).v),"
             << "Init_sol_address_t((func_user_i).v))).v;";
    expected << "(Read_Map_1(&(((self->user_c).user_b).user_arr2),"
             << "Init_sol_address_t((func_user_i).v),"
             << "Init_sol_address_t((func_user_i).v))).v;";
    expected << "(Read_Map_2(&(self->user_arr1),"
             << "Init_sol_address_t((func_user_i).v),"
             << "Init_sol_address_t((func_user_i).v))).v;";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that when the l-value of an assignment is an IndexAccess, that the
// forementioned assignment will be rewritten as a Write_ call.
BOOST_AUTO_TEST_CASE(map_assignment)
{
    char const* text = R"(
        contract A {
            struct B { int m; }
            struct C { mapping(address => int) m; }
            mapping(address => int) a;
            mapping(address => B) b;
            C c;
            mapping(address => mapping(address => int)) d;
            function f(address i) public {
                a[i] = 2;
                a[i] += 2;
                b[i].m += 2;
                c.m[i] = 2;
                d[i][i] = 3;
            }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "Write_Map_2(&(self->user_a)"
             << ",Init_sol_address_t((func_user_i).v),Init_sol_int256_t(2));";
    expected << "Write_Map_2(&(self->user_a)"
             << ",Init_sol_address_t((func_user_i).v)"
             << ",Init_sol_int256_t(((Read_Map_2(&(self->user_a)"
             << ",Init_sol_address_t((func_user_i).v))).v)+(2)));";
    expected << "(((Read_Map_3(&(self->user_b)"
             << ",Init_sol_address_t((func_user_i).v))).user_m).v"
             << ")=((((Read_Map_3(&(self->user_b)"
             << ",Init_sol_address_t((func_user_i).v))).user_m).v)+(2));";
    expected << "Write_Map_1(&((self->user_c).user_m)"
             << ",Init_sol_address_t((func_user_i).v),Init_sol_int256_t(2));";
    expected << "Write_Map_4(&(self->user_d),Init_sol_address_t((func_user_i).v),"
             << "Init_sol_address_t((func_user_i).v),Init_sol_int256_t(3));";
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
                address(a);
                int(s); uint(s);
                int(u); uint(u);
                bool(b);
                address(this);
            }
        }
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "((int)(g_literal_address_5));";
    expected << "(self->user_a).v;";
    expected << "(self->user_s).v;";
    expected << "((unsigned int)((self->user_s).v));";
    expected << "((int)((self->user_u).v));";
    expected << "(self->user_u).v;";
    expected << "(self->user_b).v;";
    expected << "((self)->model_address).v;";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "struct A_Struct_B*func_user_b__ref=&(self->user_b);";
    expected << "((func_user_b__ref)->user_i).v;";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "struct A_Struct_B*func_user_b__ref=&(self->user_b);";
    expected << "(func_user_b__ref)=(&(self->user_b));";
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

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(func, stack).convert();
    expected << "{";
    expected << "if(1){}";
    expected << "else if(0){}";
    expected << "}";
    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

// Ensures that wrapped return types are unwrapped.
BOOST_AUTO_TEST_CASE(function_call_unwraps_data)
{
    char const* text = R"(
        contract A {
            function f() public pure returns (uint) { return 5; }
            function g() public pure { f(); }
        }
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    auto func = ctrt->definedFunctions()[1];
    BOOST_CHECK_EQUAL(func->name(), "g");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expect;
    FunctionBlockConverter fbc(*func, stack);
    fbc.set_for(FunctionSpecialization(*func));
    actual << *fbc.convert();
    expect << "{"
           << "(A_Method_f(self,sender,value,blocknum,timestamp"
           << ",Init_sol_bool_t(0),origin)).v;"
           << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

BOOST_AUTO_TEST_CASE(modifier_nesting)
{
    char const* text = R"(
        contract A {
            modifier modA() {
                _;
                _;
                return;
            }
            modifier modB() {
                _;
                return;
            }
            function f() public modA() modB() pure { }
            function g() public modA() modB() { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    auto func_f = ctrt->definedFunctions()[0];
    auto func_g = ctrt->definedFunctions()[1];

    BOOST_CHECK_EQUAL(func_f->name(), "f");
    BOOST_CHECK_EQUAL(func_g->name(), "g");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    FunctionSpecialization spec_f(*func_f);
    FunctionSpecialization spec_g(*func_g);
    ModifierBlockConverter::Factory f_factory(spec_f);
    ModifierBlockConverter::Factory g_factory(spec_g);

    ostringstream f0_actual, f0_expect;
    f0_actual << *f_factory.generate(0, stack).convert();
    f0_expect << "{"
              << "A_Method_1_f(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);"
              << "A_Method_1_f(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);"
              << "return;"
              << "}";
    BOOST_CHECK_EQUAL(f0_actual.str(), f0_expect.str());

    ostringstream g0_actual, g0_expect;
    g0_actual << *g_factory.generate(0, stack).convert();
    g0_expect << "{";
    g0_expect << "A_Method_1_g(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    g0_expect << "A_Method_1_g(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    g0_expect << "return;";
    g0_expect << "}";
    BOOST_CHECK_EQUAL(g0_actual.str(), g0_expect.str());

    ostringstream f1_actual, f1_expect;
    f1_actual << *f_factory.generate(1, stack).convert();
    f1_expect << "{";
    f1_expect << "A_Method_2_f(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    f1_expect << "return;";
    f1_expect << "}";
    BOOST_CHECK_EQUAL(f1_actual.str(), f1_expect.str());

    ostringstream g1_actual, g1_expect;
    g1_actual << *g_factory.generate(1, stack).convert();
    g1_expect << "{";
    g1_expect << "A_Method_2_g(self,sender,value,blocknum,timestamp"
              << ",Init_sol_bool_t(0),origin);";
    g1_expect << "return;";
    g1_expect << "}";;
    BOOST_CHECK_EQUAL(g1_actual.str(), g1_expect.str());
}

BOOST_AUTO_TEST_CASE(modifier_retval)
{
    char const* text = R"(
        contract A {
            modifier modA() {
                _;
                return;
                _;
            }
            function f() modA() public returns (int) { return 5; }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream expected, actual;
    FunctionSpecialization spec(func);
    ModifierBlockConverter::Factory factory(spec);
    actual << *factory.generate(0, stack).convert();
    expected << "{";
    expected << "sol_int256_t func_model_rv;";
    expected << "(func_model_rv)=(A_Method_1_f(self,sender,value,blocknum,"
             << "timestamp,Init_sol_bool_t(0),origin));";
    expected << "return func_model_rv;";
    expected << "(func_model_rv)=(A_Method_1_f(self,sender,value,blocknum,"
             << "timestamp,Init_sol_bool_t(0),origin));";
    expected << "return func_model_rv;";
    expected << "}";

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(modifier_args)
{
    char const* text = R"(
        contract A {
            modifier modA(int a, int b) {
                require(a > b);
                _;
            }
            function f(int a, int b) modA(b + 5, a) public { }
        }
    )";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto const& func = *ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream expected, actual;
    FunctionSpecialization spec(func);
    ModifierBlockConverter::Factory factory(spec);
    actual << *factory.generate(0, stack).convert();
    expected << "{";
    expected << "sol_int256_t func_user_a=Init_sol_int256_t("
             << "((func_model_b).v)+(5));";
    expected << "sol_int256_t func_user_b=Init_sol_int256_t((func_model_a).v);";
    expected << "sol_require(((func_user_a).v)>((func_user_b).v),0);";
    expected << "A_Method_1_f(self,sender,value,blocknum,timestamp"
             << ",Init_sol_bool_t(0),origin,func_model_a,func_model_b);";
    expected << "}";

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(library_calls)
{
    char const* text = R"(
        library Lib {
            function incr(uint256 i) internal { i += 1; }
            function f() public pure {}
        }
		contract A {
            using Lib for uint256;
			function f(uint256 i) public {
                i.incr();
                Lib.f();
            }
		}
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto func = ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expect;
    FunctionBlockConverter fbc(*func, stack);
    fbc.set_for(FunctionSpecialization(*func));
    actual << *fbc.convert();
    expect << "{"
           << "Lib_Method_incr(Init_sol_uint256_t((func_user_i).v));"
           << "Lib_Method_f();"
           << "}";
    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

BOOST_AUTO_TEST_CASE(crypto_calls)
{
    char const* text = R"(
        contract A {
            function f(uint value, bytes32 secret, bool fake) public pure {
                keccak256(abi.encodePacked(value, fake, secret));
            }
        }
	)";

    auto const& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    auto func = ctrt->definedFunctions()[0];

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false);

    ostringstream actual, expected;
    actual << *FunctionBlockConverter(*func, stack).convert();
    expected << "{";
    expected << "sol_crypto();";
    expected << "}";

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
