/**
 * Specific tests for libsolidity/modelcheck/model/NondetSourceRegistry.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/codegen/Details.h>

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
    Model_NondetSourceRegistryTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(byte)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    NondetSourceRegistry reg(stack);
    auto raw_nd = reg.byte("Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_BYTE(0,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(range)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    NondetSourceRegistry reg(stack);
    auto raw_nd = reg.range(2, 9, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_RANGE(0,2,9,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(increase)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    NondetSourceRegistry reg(stack);
    auto var = make_shared<CIdentifier>("var", false);
    auto raw_nd = reg.increase(var, true, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_INCREASE(0,var,1,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(raw_int)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    IntegerType type(32, IntegerType::Modifier::Signed);
    NondetSourceRegistry reg(stack);
    auto raw_nd = reg.raw_val(type, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_INT(0,32,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(raw_uint)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    IntegerType type(32, IntegerType::Modifier::Unsigned);
    NondetSourceRegistry reg(stack);
    auto raw_nd = reg.raw_val(type, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_UINT(0,32,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(raw_addr)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 8, true, false);

    AddressType type(StateMutability::Payable);
    NondetSourceRegistry reg(stack);
    auto raw_nd = reg.raw_val(type, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_RANGE(0,0,10,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(raw_bool)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 0, false, false);

    BoolType type;
    NondetSourceRegistry reg(stack);
    auto raw_nd = reg.raw_val(type, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "GET_ND_RANGE(0,0,2,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(unique_sources)
{
    char const* text = "contract X {}";
    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "X");

    vector<ContractDefinition const*> model({ ctrt });
    vector<SourceUnit const*> full({ &unit });
    auto stack = make_shared<AnalysisStack>(model, full, 8, true, false);

    auto var = make_shared<CIdentifier>("var", false);
    IntegerType uint_type(32, IntegerType::Modifier::Unsigned);
    IntegerType sint_type(32, IntegerType::Modifier::Signed);
    AddressType addr_type(StateMutability::Payable);
    BoolType bool_type;

    NondetSourceRegistry reg(stack);
    auto nd_0 = reg.byte("Blah");
    auto nd_1 = reg.range(2, 9, "Blah");
    auto nd_2 = reg.increase(var, true, "Blah");
    auto nd_3 = reg.raw_val(uint_type, "Blah");
    auto nd_4 = reg.raw_val(sint_type, "Blah");
    auto nd_5 = reg.raw_val(addr_type, "Blah");
    auto nd_6 = reg.raw_val(bool_type, "Blah");
    auto nd_7 = reg.byte("Blah");
    auto nd_8 = reg.range(2, 9, "Blah");
    auto nd_9 = reg.increase(var, true, "Blah");
    auto nd_10 = reg.raw_val(uint_type, "Blah");
    auto nd_11 = reg.raw_val(sint_type, "Blah");
    auto nd_12 = reg.raw_val(addr_type, "Blah");
    auto nd_13 = reg.raw_val(bool_type, "Blah");

    std::ostringstream expr_0;
    std::ostringstream expr_1;
    std::ostringstream expr_2;
    std::ostringstream expr_3;
    std::ostringstream expr_4;
    std::ostringstream expr_5;
    std::ostringstream expr_6;
    std::ostringstream expr_7;
    std::ostringstream expr_8;
    std::ostringstream expr_9;
    std::ostringstream expr_10;
    std::ostringstream expr_11;
    std::ostringstream expr_12;
    std::ostringstream expr_13;

    expr_0 << *nd_0;
    expr_1 << *nd_1;
    expr_2 << *nd_2;
    expr_3 << *nd_3;
    expr_4 << *nd_4;
    expr_5 << *nd_5;
    expr_6 << *nd_6;
    expr_7 << *nd_7;
    expr_8 << *nd_8;
    expr_9 << *nd_9;
    expr_10 << *nd_10;
    expr_11 << *nd_11;
    expr_12 << *nd_12;
    expr_13 << *nd_13;

    BOOST_CHECK_EQUAL(expr_0.str(), "GET_ND_BYTE(0,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_1.str(), "GET_ND_RANGE(1,2,9,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_2.str(), "GET_ND_INCREASE(2,var,1,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_3.str(), "GET_ND_UINT(3,32,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_4.str(), "GET_ND_INT(4,32,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_5.str(), "GET_ND_RANGE(5,0,10,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_6.str(), "GET_ND_RANGE(6,0,2,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_7.str(), "GET_ND_BYTE(7,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_8.str(), "GET_ND_RANGE(8,2,9,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_9.str(), "GET_ND_INCREASE(9,var,1,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_10.str(), "GET_ND_UINT(10,32,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_11.str(), "GET_ND_INT(11,32,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_12.str(), "GET_ND_RANGE(12,0,10,\"Blah\")");
    BOOST_CHECK_EQUAL(expr_13.str(), "GET_ND_RANGE(13,0,2,\"Blah\")");
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
