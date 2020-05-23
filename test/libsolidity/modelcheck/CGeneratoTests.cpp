/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/SimpleCGenerator.cpp
 */

#include <libsolidity/modelcheck/codegen/Details.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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

void test_switch_case(CSwitch & switch_stmt, string const& default_case)
{
    ostringstream default_actual, default_expect;
    default_actual << switch_stmt;
    default_expect << "switch(cond){"
                   << "default:{" << default_case << "}"
                   << "}";
    BOOST_CHECK_EQUAL(default_actual.str(), default_expect.str());

    CBlockList body_3{make_shared<CReturn>(make_shared<CIntLiteral>(3))};
    ostringstream one_case_actual, one_case_expect;
    switch_stmt.add_case(3, body_3);
    one_case_actual << switch_stmt;
    one_case_expect << "switch(cond){"
                    << "case 3:{return 3;}"
                    << "default:{" << default_case << "}"
                    << "}";
    BOOST_CHECK_EQUAL(one_case_actual.str(), one_case_expect.str());

    CBlockList body_5{make_shared<CReturn>(make_shared<CIntLiteral>(5))};
    ostringstream two_case_actual, two_case_expect;
    switch_stmt.add_case(5, body_5);
    two_case_actual << switch_stmt;
    two_case_expect << "switch(cond){"
                    << "case 3:{return 3;}"
                    << "case 5:{return 5;}"
                    << "default:{" << default_case << "}"
                    << "}";
    BOOST_CHECK_EQUAL(two_case_actual.str(), two_case_expect.str());
}

// -------------------------------------------------------------------------- //

BOOST_FIXTURE_TEST_SUITE(
    CGeneratorTests,
    ::dev::solidity::test::AnalysisFramework
)

// Tests that the switch is built properly, without a default.
BOOST_AUTO_TEST_CASE(switch_without_default)
{
    CSwitch switch_stmt(make_shared<CIdentifier>("cond", false));
    test_switch_case(switch_stmt, "break;");

}

// Tests that the switch is built properly, without a default.
BOOST_AUTO_TEST_CASE(switch_with_default)
{
    CBlockList body{make_shared<CReturn>(make_shared<CIntLiteral>(5))};
    CSwitch switch_stmt(make_shared<CIdentifier>("cond", false), move(body));
    test_switch_case(switch_stmt, "return 5;");
}

// Tests the various variable declarations.
BOOST_AUTO_TEST_CASE(var_decl_types)
{
    CVarDecl basic1("type", "name");
    CVarDecl basic2("type", "name", false);
    CVarDecl ptr("type", "name", true);
    CVarDecl set_val("type", "name", false, make_shared<CIntLiteral>(42));

    ostringstream basic1_actual;
    basic1_actual << basic1;
    BOOST_CHECK_EQUAL(basic1_actual.str(), "type name;");

    ostringstream basic2_actual;
    basic2_actual << basic2;
    BOOST_CHECK_EQUAL(basic2_actual.str(), "type name;");

    ostringstream ptr_actual;
    ptr_actual << ptr;
    BOOST_CHECK_EQUAL(ptr_actual.str(), "type*name;");

    ostringstream set_val_actual;
    set_val_actual << set_val;
    BOOST_CHECK_EQUAL(set_val_actual.str(), "type name=42;");
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
