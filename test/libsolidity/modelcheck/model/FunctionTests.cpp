/**
 * Specific tests for libsolidity/modelcheck/model/Function.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/model/Function.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>

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
    Model_FunctionTests, ::dev::solidity::test::AnalysisFramework
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
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream actual, expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    expect << "void Init_A(struct A*self,sol_address_t sender,sol_uint256_t "
           << "value,sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t "
           << "paid,sol_address_t origin)";
    expect << "{";
    expect << "((self)->model_balance)=(Init_sol_uint256_t(0));";
    expect << "}";
    expect << "sol_uint40_t A_Method_f(struct A*self,sol_address_t sender"
           << ",sol_uint256_t value,sol_uint256_t blocknum,sol_uint256_t "
           << "timestamp,sol_bool_t paid,sol_address_t origin)";
    expect << "{";
    expect << "{return Init_sol_uint40_t(20);}";
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
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream actual, expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    expect << "void Init_A(struct A*self,sol_address_t sender,sol_uint256_t "
           << "value,sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t "
           << "paid,sol_address_t origin)";
    expect << "{";
    expect << "((self)->model_balance)=(Init_sol_uint256_t(0));";
    expect << "}";
    expect << "sol_uint40_t A_Method_f(struct A*self,sol_address_t sender"
           << ",sol_uint256_t value,sol_uint256_t blocknum,sol_uint256_t "
           << "timestamp,sol_bool_t paid,sol_address_t origin)";
    expect << "{";
    expect << "if(((paid).v)==(1))(((self)->model_balance).v)+=((value).v);";
    expect << "{return Init_sol_uint40_t(20);}";
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
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream actual, expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- InitZero_A_Struct_B
    expect << "struct A_Struct_B ZeroInit_A_Struct_B(void)";
    expect << "{";
    expect << "struct A_Struct_B tmp;";
    expect << "((tmp).user_a)=(Init_sol_uint256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_Struct_B
    expect << "struct A_Struct_B Init_A_Struct_B(sol_uint256_t user_a)";
    expect << "{";
    expect << "struct A_Struct_B tmp=ZeroInit_A_Struct_B();";
    expect << "((tmp).user_a)=(user_a);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_Struct_B
    expect << "struct A_Struct_B ND_A_Struct_B(void)";
    expect << "{";
    expect << "struct A_Struct_B tmp;";
    expect << "((tmp).user_a)=(Init_sol_uint256_t(GET_ND_UINT(0,256,\"A_Struct_B:a\")));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A
    expect << "void Init_A(struct A*self,sol_address_t sender,sol_uint256_t "
           << "value,sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t "
           << "paid,sol_address_t origin)";
    expect << "{";
    expect << "((self)->model_balance)=(Init_sol_uint256_t(0));";
    expect << "((self)->user_a)=(Init_sol_uint256_t(0));";
    expect << "((self)->user_b)=(Init_sol_uint256_t(10));";
    expect << "((self)->user_c)=(ZeroInit_A_Struct_B());";
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
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream actual, expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- Ctor_A
    expect << "void A_Constructor(struct A*self,sol_address_t sender"
           << ",sol_uint256_t value,sol_uint256_t blocknum"
           << ",sol_uint256_t timestamp,sol_bool_t paid,sol_address_t origin"
           << ",sol_uint256_t func_user___a)";
    expect << "{";
    expect << "((self->user_a).v)=((func_user___a).v);";
    expect << "}";
    // -- Init_A
    expect << "void Init_A(struct A*self,sol_address_t sender,sol_uint256_t "
           << "value,sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t "
           << "paid,sol_address_t origin,sol_uint256_t user___a)";
    expect << "{";
    expect << "((self)->model_balance)=(Init_sol_uint256_t(0));";
    expect << "((self)->user_a)=(Init_sol_uint256_t(0));";
    expect << "((self)->user_b)=(Init_sol_uint256_t(0));";
    expect << "A_Constructor(self,sender,value,blocknum,timestamp"
           << ",Init_sol_bool_t(0),origin,user___a);";
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
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream actual, expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::FULL, false
    ).print(actual);
    // -- InitZero_A_Struct_B
    expect << "struct A_Struct_B ZeroInit_A_Struct_B(void)";
    expect << "{";
    expect << "struct A_Struct_B tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(0));";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_Struct_B
    expect << "struct A_Struct_B Init_A_Struct_B(sol_int256_t user_i1)";
    expect << "{";
    expect << "struct A_Struct_B tmp=ZeroInit_A_Struct_B();";
    expect << "((tmp).user_i1)=(user_i1);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_Struct_B
    expect << "struct A_Struct_B ND_A_Struct_B(void)";
    expect << "{";
    expect << "struct A_Struct_B tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(GET_ND_INT(0,256,\"A_Struct_B:i1\")));";
    expect << "return tmp;";
    expect << "}";
    // -- InitZero_A_Struct_C
    expect << "struct A_Struct_C ZeroInit_A_Struct_C(void)";
    expect << "{";
    expect << "struct A_Struct_C tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(0));";
    expect << "((tmp).user_b1)=(ZeroInit_A_Struct_B());";
    expect << "((tmp).user_i2)=(Init_sol_int256_t(0));";
    expect << "((tmp).user_ui1)=(Init_sol_uint256_t(0));";
    expect << "((tmp).user_b2)=(ZeroInit_A_Struct_B());";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A_Struct_C
    expect << "struct A_Struct_C Init_A_Struct_C(sol_int256_t user_i1"
              ",sol_int256_t user_i2,sol_uint256_t user_ui1)";
    expect << "{";
    expect << "struct A_Struct_C tmp=ZeroInit_A_Struct_C();";
    expect << "((tmp).user_i1)=(user_i1);";
    expect << "((tmp).user_i2)=(user_i2);";
    expect << "((tmp).user_ui1)=(user_ui1);";
    expect << "return tmp;";
    expect << "}";
    // -- ND_A_Struct_C
    expect << "struct A_Struct_C ND_A_Struct_C(void)";
    expect << "{";
    expect << "struct A_Struct_C tmp;";
    expect << "((tmp).user_i1)=(Init_sol_int256_t(GET_ND_INT(1,256,\"A_Struct_C:i1\")));";
    expect << "((tmp).user_b1)=(ND_A_Struct_B());";
    expect << "((tmp).user_i2)=(Init_sol_int256_t(GET_ND_INT(2,256,\"A_Struct_C:i2\")));";
    expect << "((tmp).user_ui1)=(Init_sol_uint256_t(GET_ND_UINT(3,256,\"A_Struct_C:ui1\")));";
    expect << "((tmp).user_b2)=(ND_A_Struct_B());";
    expect << "return tmp;";
    expect << "}";
    // -- Init_A
    expect << "void Init_A(struct A*self,sol_address_t sender,sol_uint256_t "
           << "value,sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t"
           << " paid,sol_address_t origin)";
    expect << "{";
    expect << "((self)->model_balance)=(Init_sol_uint256_t(0));";
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
            mapping(address => int) m;
            function g() private pure {}
            function f() public pure { g(); }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "A");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream ext_actual, ext_expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::EXT, true
    ).print(ext_actual);
    ext_expect << "void Init_A(struct A*self,sol_address_t sender,sol_uint256_t"
               << " value,sol_uint256_t blocknum,sol_uint256_t timestamp,"
               << "sol_bool_t paid,sol_address_t origin);";
    ext_expect << "void A_Method_f(struct A*self,sol_address_t sender"
               << ",sol_uint256_t value,sol_uint256_t blocknum,sol_uint256_t "
               << "timestamp,sol_bool_t paid,sol_address_t origin);";

    ostringstream int_actual, int_expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::INT, true
    ).print(int_actual);
    int_expect << "struct A_Struct_B ZeroInit_A_Struct_B(void);";
    int_expect << "struct A_Struct_B Init_A_Struct_B(sol_int256_t user_i);";
    int_expect << "struct A_Struct_B ND_A_Struct_B(void);";
    int_expect << "struct Map_1 ZeroInit_Map_1(void);";
    int_expect << "sol_int256_t Read_Map_1(struct Map_1*arr,sol_address_t "
               << "key_0);";
    int_expect << "void Write_Map_1(struct Map_1*arr,sol_address_t key_0"
               << ",sol_int256_t dat);";
    int_expect << "void A_Method_g(struct A*self,sol_address_t sender"
               << ",sol_uint256_t value,sol_uint256_t blocknum,sol_uint256_t "
               << "timestamp,sol_bool_t paid,sol_address_t origin);";

    BOOST_CHECK_EQUAL(ext_actual.str(), ext_expect.str());
    BOOST_CHECK_EQUAL(int_actual.str(), int_expect.str());
}

// Ensures we filter out irrelevant methods.
BOOST_AUTO_TEST_CASE(can_hide_unused_externals)
{
    char const* text = R"(
        contract A {}
        contract B is A {}
        contract C {
            A a;
            constructor() public { a = new B(); }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "C");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream ext_actual, ext_expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::EXT, true
    ).print(ext_actual);
    ext_expect << "void C_Constructor(struct C*self,sol_address_t sender"
               << ",sol_uint256_t value,sol_uint256_t blocknum"
               << ",sol_uint256_t timestamp,sol_bool_t paid"
               << ",sol_address_t origin);";
    ext_expect << "void Init_C(struct C*self,sol_address_t sender"
               << ",sol_uint256_t value,sol_uint256_t blocknum"
               << ",sol_uint256_t timestamp,sol_bool_t paid"
               << ",sol_address_t origin);";
    ext_expect << "void Init_A_For_B(struct B*self,sol_address_t sender"
               << ",sol_uint256_t value,sol_uint256_t blocknum"
               << ",sol_uint256_t timestamp,sol_bool_t paid"
               << ",sol_address_t origin);";
    ext_expect << "void Init_B(struct B*self,sol_address_t sender"
               << ",sol_uint256_t value,sol_uint256_t blocknum"
               << ",sol_uint256_t timestamp,sol_bool_t paid"
               << ",sol_address_t origin);";

    BOOST_CHECK_EQUAL(ext_actual.str(), ext_expect.str());
}

BOOST_AUTO_TEST_CASE(inherited_duplicates)
{
    char const* text = R"(
        contract A {
            struct X {
                int i;
            }
            mapping(address => int) m;
        }
        contract B is A {}
        contract C is A {}
    )";

    auto const &ast = *parseAndAnalyse(text);
    auto ctrt_b = retrieveContractByName(ast, "B");
    auto ctrt_c = retrieveContractByName(ast, "C");

    vector<ContractDefinition const*> model({ ctrt_b, ctrt_c });
    vector<SourceUnit const*> full({ &ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 0;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);

    ostringstream actual, expect;
    FunctionConverter(
        stack, nd_reg, false, 1, FunctionConverter::View::INT, true
    ).print(actual);
    expect << "struct A_Struct_X ZeroInit_A_Struct_X(void);";
    expect << "struct A_Struct_X Init_A_Struct_X(sol_int256_t user_i);";
    expect << "struct A_Struct_X ND_A_Struct_X(void);";
    expect << "struct Map_1 ZeroInit_Map_1(void);";
    expect << "sol_int256_t Read_Map_1(struct Map_1*arr,sol_address_t key_0);";
    expect << "void Write_Map_1(struct Map_1*arr,sol_address_t key_0"
           << ",sol_int256_t dat);";

    BOOST_CHECK_EQUAL(actual.str(), expect.str());
}

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
