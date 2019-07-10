/**
 * @date 2019
 * Tests resolution of variable names within function translations.
 */

#include <libsolidity/modelcheck/VariableScopeResolver.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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

BOOST_FIXTURE_TEST_SUITE(
    VariableScopeResolution,
    ::dev::solidity::test::AnalysisFramework
)

// Tests that without a scope, resolution is exception free. The variable `var`
// should be remapped onto a state variable `self->d_var`.
BOOST_AUTO_TEST_CASE(member_variables_no_scope)
{
    VariableScopeResolver resolver;

    Identifier id(langutil::SourceLocation(), make_shared<string>("var"));
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id), "self->d_var");
}

// Tests that a scope may be created, and that a declaration, `a`, may be
// bound to it without exception. If this succeeds, it is then verified that `a`
// resolves to `a` while `var` resolves to `self->d_var`.
BOOST_AUTO_TEST_CASE(member_variables_scoped)
{
    char const* text = R"(
		contract A {
            int a;
		}
	)";

    auto const &unit = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(unit, "A");
    auto const &decl = *ctrt.stateVariables()[0];

    VariableScopeResolver resolver;
    resolver.enter();
    resolver.record_declaration(decl);

    Identifier id1(langutil::SourceLocation(), make_shared<string>("var"));
    Identifier id2(langutil::SourceLocation(), make_shared<string>("a"));
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id1), "self->d_var");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id2), "a");
}

// Attempts to produce three scopes, one declaring `a`, one declaring `b`, and
// one declaring `c`. Each scope is then popped. The resolution of `a`, `b` and
// `c` in each such context is validated. This ensures scopes may be added and
// removed, and that scopes are popped in the correct order.
BOOST_AUTO_TEST_CASE(local_variables_scoped)
{
    char const* text = R"(
		contract A {
            int a;
            int b;
            int c;
		}
	)";

    auto const &unit = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(unit, "A");
    auto const &decl_a = *ctrt.stateVariables()[0];
    auto const &decl_b = *ctrt.stateVariables()[1];
    auto const &decl_c = *ctrt.stateVariables()[2];

    VariableScopeResolver resolver;
    Identifier id_a(langutil::SourceLocation(), make_shared<string>("a"));
    Identifier id_b(langutil::SourceLocation(), make_shared<string>("b"));
    Identifier id_c(langutil::SourceLocation(), make_shared<string>("c"));

    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "self->d_a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "self->d_b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "self->d_c");

    resolver.enter();
    resolver.record_declaration(decl_a);
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "self->d_b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "self->d_c");

    resolver.enter();
    resolver.record_declaration(decl_b);
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "self->d_c");

    resolver.enter();
    resolver.record_declaration(decl_c);
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "c");

    resolver.exit();
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "self->d_c");

    resolver.exit();
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "self->d_b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "self->d_c");

    resolver.exit();
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_a), "self->d_a");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_b), "self->d_b");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(id_c), "self->d_c");
}

// A single scope is set up, with a single declaration. The resolutions of
// verious special-case keywords are validated.
BOOST_AUTO_TEST_CASE(state_resolution)
{
    char const* text = R"(
		contract A {
            int a;
		}
	)";

    auto const &unit = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(unit, "A");
    auto const &decl = *ctrt.stateVariables()[0];

    VariableScopeResolver resolver;
    resolver.enter();
    resolver.record_declaration(decl);

    Identifier txn(langutil::SourceLocation(), make_shared<string>("tx"));
    Identifier msg(langutil::SourceLocation(), make_shared<string>("msg"));
    Identifier blk(langutil::SourceLocation(), make_shared<string>("block"));
    Identifier ths(langutil::SourceLocation(), make_shared<string>("this"));
    Identifier now(langutil::SourceLocation(), make_shared<string>("now"));
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(txn), "*state");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(msg), "*state");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(blk), "*state");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(ths), "*self");
    BOOST_CHECK_EQUAL(resolver.resolve_identifier(now), "state->blocknum");
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
