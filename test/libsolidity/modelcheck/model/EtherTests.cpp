/**
 * Tests for libsolidity/modelcheck/utils/LibVerify.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/model/Ether.h>

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
    Model_EtherTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(detects_payments)
{
    char const* text = R"(
        contract A {
            function f(address payable _a) public {
                require(_a.send(5));
            }
        }
        contract B {
            function f(address payable _a) public {
                _a.transfer(5);
            }
        }
        contract C {
            function f() public payable {}
        }
    )";

    auto ast = parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(*ast, "A");
    auto ctrt_b = retrieveContractByName(*ast, "B");
    auto ctrt_c = retrieveContractByName(*ast, "C");

    vector<SourceUnit const*> full({ ast });

    AnalysisSettings settings;
    settings.persistent_user_count = 1;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;

    ostringstream output_a;
    vector<ContractDefinition const*> model_a({ ctrt_a });
    auto stack_a = make_shared<AnalysisStack>(model_a, full, settings);
    auto nd_reg_a = make_shared<NondetSourceRegistry>(stack_a);
    EtherMethodGenerator gen_a(stack_a, nd_reg_a);
    gen_a.print(output_a, true);
    auto out_a = output_a.str();
    BOOST_CHECK(out_a.find("sol_send") != string::npos);
    BOOST_CHECK(out_a.find("sol_transfer") == string::npos);
    BOOST_CHECK(out_a.find("sol_pay") == string::npos);

    ostringstream output_b;
    vector<ContractDefinition const*> model_b({ ctrt_b });
    auto stack_b = make_shared<AnalysisStack>(model_b, full, settings);
    auto nd_reg_b = make_shared<NondetSourceRegistry>(stack_b);
    EtherMethodGenerator gen_b(stack_b, nd_reg_b);
    gen_b.print(output_b, true);
    auto out_b = output_b.str();
    BOOST_CHECK(out_b.find("sol_send") != string::npos);
    BOOST_CHECK(out_b.find("sol_transfer") != string::npos);
    BOOST_CHECK(out_b.find("sol_pay") == string::npos);

    ostringstream output_c;
    vector<ContractDefinition const*> model_c({ ctrt_c });
    auto stack_c = make_shared<AnalysisStack>(model_c, full, settings);
    auto nd_reg_c = make_shared<NondetSourceRegistry>(stack_c);
    EtherMethodGenerator gen_c(stack_c, nd_reg_c);
    gen_c.print(output_c, true);
    auto out_c = output_c.str();
    BOOST_CHECK(out_c.find("sol_send") == string::npos);
    BOOST_CHECK(out_c.find("sol_transfer") == string::npos);
    BOOST_CHECK(out_c.find("sol_pay") != string::npos);
}

BOOST_AUTO_TEST_CASE(handles_contract_addresses)
{
    char const* text = R"(
        contract A {}
        contract B {
            function() external {}
        }
        contract C {
            function() external payable {
                require(msg.sender.send(1));
            }
        }
    )";

    auto ast = parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(*ast, "A");
    auto ctrt_b = retrieveContractByName(*ast, "B");
    auto ctrt_c = retrieveContractByName(*ast, "C");

    vector<SourceUnit const*> full({ ast });
    vector<ContractDefinition const*> model({ ctrt_a, ctrt_b, ctrt_c });

    AnalysisSettings settings;
    settings.persistent_user_count = 1;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);

    ostringstream actual;
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);
    EtherMethodGenerator gen(stack, nd_reg);
    gen.print(actual, false);

    ostringstream expect;
    expect << "uint8_t sol_send(sol_address_t sender,sol_uint256_t value,"
           << "sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t paid,"
           << "sol_address_t origin,sol_address_t src,sol_uint256_t*bal,"
           << "sol_address_t dst,sol_uint256_t amt)"
           << "{"
           << "if(((bal)->v)<((amt).v))return 0;"
           << "if(((dst).v)==(0)){return 0;}"
           << "if(((dst).v)==(1)){return 0;}"
           << "if(((dst).v)==(2)){return 0;}"
           << "if(((dst).v)==(3)){sol_assert(0,\"Fallback not allowed in: "
           << "C\");}"
           << "((bal)->v)-=((amt).v);"
           << "return GET_ND_BYTE";

    BOOST_CHECK(actual.str().find(expect.str()) != string::npos);
}

BOOST_AUTO_TEST_CASE(handles_nested_contracts)
{
    char const* text = R"(
        contract A {
            function() external payable {
                require(msg.sender.send(1));
            }
        }
        contract B {
            A a;
            constructor() public {
                a = new A();
            }
        }
    )";

    auto ast = parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(*ast, "B");

    vector<SourceUnit const*> full({ ast });
    vector<ContractDefinition const*> model({ ctrt });

    AnalysisSettings settings;
    settings.persistent_user_count = 1;
    settings.use_concrete_users = false;
    settings.use_global_contracts = false;
    settings.escalate_reqs = false;
    auto stack = make_shared<AnalysisStack>(model, full, settings);

    ostringstream actual;
    auto nd_reg = make_shared<NondetSourceRegistry>(stack);
    EtherMethodGenerator gen(stack, nd_reg);
    gen.print(actual, false);

    ostringstream expect;
    expect << "uint8_t sol_send(sol_address_t sender,sol_uint256_t value,"
           << "sol_uint256_t blocknum,sol_uint256_t timestamp,sol_bool_t paid,"
           << "sol_address_t origin,sol_address_t src,sol_uint256_t*bal,"
           << "sol_address_t dst,sol_uint256_t amt)"
           << "{"
           << "if(((bal)->v)<((amt).v))return 0;"
           << "if(((dst).v)==(0)){return 0;}"
           << "if(((dst).v)==(1)){return 0;}"
           << "if(((dst).v)==(2)){sol_assert(0,\"Fallback not allowed in: "
           << "A\");}"
           << "((bal)->v)-=((amt).v);"
           << "return GET_ND_BYTE";

    BOOST_CHECK(actual.str().find(expect.str()) != string::npos);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
