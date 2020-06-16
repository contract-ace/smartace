/**
 * Specific tests for libsolidity/modelcheck/utils/AST.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/utils/AST.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

using namespace std;
using namespace langutil;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

// -------------------------------------------------------------------------- //

using SubD = Literal::SubDenomination;

BOOST_FIXTURE_TEST_SUITE(
    Utils_ASTTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(find_match)
{
    char const* text = R"(
        contract A {
            struct a { int a; }
            int b;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "A");

    auto var_a = find_named_match<VariableDeclaration>(ctrt, "a");
    auto var_b = find_named_match<VariableDeclaration>(ctrt, "b");
    auto struct_a = find_named_match<StructDefinition>(ctrt, "a");
    auto struct_b = find_named_match<StructDefinition>(ctrt, "b");

    BOOST_CHECK_EQUAL(var_a, nullptr);
    BOOST_CHECK_EQUAL(var_b, ctrt->stateVariables()[0]);
    BOOST_CHECK_EQUAL(struct_a, ctrt->definedStructs()[0]);
    BOOST_CHECK_EQUAL(struct_b, nullptr);
}

// Tests the node sniffer utility with several types. Ensures that it ignores
// control-flow sub-expressions.
BOOST_AUTO_TEST_CASE(node_sniffer)
{
    auto id = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a")
    );

    auto m1 = make_shared<MemberAccess>(SourceLocation(), id, nullptr);
    auto m2 = make_shared<MemberAccess>(SourceLocation(), m1, nullptr);

    auto lit = make_shared<Literal>(
        SourceLocation(), Token::As, nullptr, SubD::None
    );

    TupleExpression tuple(SourceLocation(), {m2}, false);

    Conditional cond(SourceLocation(), lit, id, id);

    IndexAccess indx(SourceLocation(), id, lit);

    FunctionCall call(SourceLocation(), id, {lit}, {nullptr});

    // Checks lookup.
    BOOST_CHECK_EQUAL(NodeSniffer<MemberAccess>(tuple).find(), m2.get());
    BOOST_CHECK_EQUAL(NodeSniffer<MemberAccess>(*m2).find(), m2.get());
    BOOST_CHECK_EQUAL(NodeSniffer<MemberAccess>(*m1).find(), m1.get());
    BOOST_CHECK_EQUAL(NodeSniffer<MemberAccess>(*id).find(), nullptr);
    BOOST_CHECK_EQUAL(NodeSniffer<Identifier>(tuple).find(), id.get());

    // Checks ignored parts.
    BOOST_CHECK_EQUAL(NodeSniffer<Literal>(cond).find(), nullptr);
    BOOST_CHECK_EQUAL(NodeSniffer<Literal>(indx).find(), nullptr);
    BOOST_CHECK_EQUAL(NodeSniffer<Literal>(indx).find(), nullptr);
}

// Tests that the l-value sniffer supports various l-value nodes, and that
// search depth is limited to that of the first l-value.
BOOST_AUTO_TEST_CASE(lvalue_sniffer)
{
    auto id = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a")
    );

    auto lit = make_shared<Literal>(
        SourceLocation(), Token::As, nullptr, SubD::None
    );

    auto m1 = make_shared<MemberAccess>(SourceLocation(), id, nullptr);

    Conditional cond(SourceLocation(), id, lit, lit);

    IndexAccess indx(SourceLocation(), id, id);

    FunctionCall call(SourceLocation(), m1, {m1}, {nullptr});

    // Tests that each lookup works.
    BOOST_CHECK_EQUAL(LValueSniffer<MemberAccess>(*m1).find(), m1.get());
    BOOST_CHECK_EQUAL(LValueSniffer<IndexAccess>(indx).find(), &indx);
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(*id).find(), id.get());

    // Ensures that only the first l-value is checked.
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(*m1).find(), nullptr);
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(indx).find(), nullptr);

    // Checks that branches which cannot return l-values are pruned.
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(call).find(), nullptr);
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(indx).find(), nullptr);
}

BOOST_AUTO_TEST_CASE(expr_cleaner)
{
    char const* text = R"(
        contract A {
            function f() public pure {
                (((((5)))));
                (((5)));
                5;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "A");
    auto body = ctrt->definedFunctions()[0]->body().statements();

    for (auto stmt : ctrt->definedFunctions()[0]->body().statements())
    {
        auto expr_stmt = dynamic_cast<ExpressionStatement const*>(stmt.get());
        auto const& clean = ExpressionCleaner(expr_stmt->expression()).clean();
        BOOST_CHECK_NE(dynamic_cast<Literal const*>(&clean), nullptr);
    }
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
