/**
 * Tests for libsolidity/modelcheck/utils/LibVerify.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/model/Ether.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>

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

    ostringstream output_a;
    vector<ContractDefinition const*> model_a({ ctrt_a });
    auto stack_a = make_shared<AnalysisStack>(model_a, full, 1, false);
    EtherMethodGenerator gen_a(stack_a);
    gen_a.print(output_a, true);
    auto out_a = output_a.str();
    BOOST_CHECK(out_a.find("sol_send") != string::npos);
    BOOST_CHECK(out_a.find("sol_transfer") == string::npos);
    BOOST_CHECK(out_a.find("sol_pay") == string::npos);

    ostringstream output_b;
    vector<ContractDefinition const*> model_b({ ctrt_b });
    auto stack_b = make_shared<AnalysisStack>(model_b, full, 1, false);
    EtherMethodGenerator gen_b(stack_b);
    gen_b.print(output_b, true);
    auto out_b = output_b.str();
    BOOST_CHECK(out_b.find("sol_send") != string::npos);
    BOOST_CHECK(out_b.find("sol_transfer") != string::npos);
    BOOST_CHECK(out_b.find("sol_pay") == string::npos);

    ostringstream output_c;
    vector<ContractDefinition const*> model_c({ ctrt_c });
    auto stack_c = make_shared<AnalysisStack>(model_c, full, 1, false);
    EtherMethodGenerator gen_c(stack_c);
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

    ostringstream actual;
    auto stack = make_shared<AnalysisStack>(model, full, 1, false);
    EtherMethodGenerator gen(stack);
    gen.print(actual, false);

    ostringstream expect;
    expect << "uint8_t sol_send(sol_uint256_t*bal,sol_address_t dst"
           << ",sol_uint256_t amt)"
           << "{"
           << "if(((bal)->v)<((amt).v))return 0;"
           << "if(((dst).v)==(0)){return 0;}"
           << "if(((dst).v)==(1)){return 0;}"
           << "if(((dst).v)==(2)){return 0;}"
           << "if(((dst).v)==(3)){sol_assert(0,0);}"
           << "((bal)->v)-=((amt).v);"
           << "return nd_byte";

    BOOST_CHECK(actual.str().find(expect.str()) != string::npos);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
