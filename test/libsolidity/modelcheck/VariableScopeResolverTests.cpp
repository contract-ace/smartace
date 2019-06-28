/**
 * @date 2019
 * Tests resolution of variable names within function translations.
 */

#include <libsolidity/modelcheck/VariableScopeResolver.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <string>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(VariableScopeResolution, ::dev::solidity::test::AnalysisFramework)

BOOST_AUTO_TEST_CASE(member_variables_no_scope)
{
    VariableScopeResolver resolver;

    Identifier id(langutil::SourceLocation(), make_shared<string>("var"));
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_var");
}

BOOST_AUTO_TEST_CASE(member_variables_scoped)
{
    char const* text = R"(
		contract A {
            int a;
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &decl = *ctrt.stateVariables()[0];

    VariableScopeResolver resolver;
    resolver.enter();
    resolver.record_declaration(decl);

    Identifier id(langutil::SourceLocation(), make_shared<string>("var"));
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_var");
}

BOOST_AUTO_TEST_CASE(local_variables_scoped)
{
    char const* text = R"(
		contract A {
            int a;
            int b;
            int c;
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &decl_a = *ctrt.stateVariables()[0];
    const auto &decl_b = *ctrt.stateVariables()[1];
    const auto &decl_c = *ctrt.stateVariables()[2];

    VariableScopeResolver resolver;
    Identifier id(langutil::SourceLocation(), make_shared<string>("c"));

    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_c");

    resolver.enter();
    resolver.record_declaration(decl_a);
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_c");

    resolver.enter();
    resolver.record_declaration(decl_b);
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_c");

    resolver.enter();
    resolver.record_declaration(decl_c);
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "c");

    resolver.exit();
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_c");

    resolver.exit();
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_c");

    resolver.exit();
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_c");
}

BOOST_AUTO_TEST_CASE(state_resolution)
{
    char const* text = R"(
		contract A {
            int a;
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &decl = *ctrt.stateVariables()[0];

    VariableScopeResolver resolver;
    resolver.enter();
    resolver.record_declaration(decl);

    Identifier txn(langutil::SourceLocation(), make_shared<string>("tx"));
    Identifier msg(langutil::SourceLocation(), make_shared<string>("msg"));
    Identifier blk(langutil::SourceLocation(), make_shared<string>("block"));
    Identifier ths(langutil::SourceLocation(), make_shared<string>("this"));
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(txn), "state");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(msg), "state");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(blk), "state");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(ths), "self");
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
