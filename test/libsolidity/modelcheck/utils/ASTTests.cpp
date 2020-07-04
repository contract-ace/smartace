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

BOOST_AUTO_TEST_CASE(decl_is_ref_works)
{
    auto viz = Declaration::Visibility::Default;
    auto storage = VariableDeclaration::Storage;
    auto calldata =VariableDeclaration::CallData;
    auto memory =VariableDeclaration::Memory;

    VariableDeclaration d_1(SourceLocation(), 0, 0, 0, viz, 1, 1, 1, storage);
    VariableDeclaration d_2(SourceLocation(), 0, 0, 0, viz, 1, 1, 1, calldata);
    VariableDeclaration d_3(SourceLocation(), 0, 0, 0, viz, 1, 1, 1, memory);

    BOOST_CHECK(decl_is_ref(d_1));
    BOOST_CHECK(!decl_is_ref(d_2));
    BOOST_CHECK(!decl_is_ref(d_3));
}

BOOST_AUTO_TEST_CASE(node_to_ref_works)
{
    char const* text = R"(
        contract A {}
        contract B {
            struct C {
                int d;
            }
            A a;
            C c;
            constructor() public {
                a = new A();
                c = C(1);
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt_a = retrieveContractByName(ast, "A");
    auto ctrt_b = retrieveContractByName(ast, "B");

    auto func = ctrt_b->definedFunctions()[0];
    auto stmts = func->body().statements();
    auto stmt_1 = dynamic_cast<ExpressionStatement const*>(stmts[0].get());
    auto stmt_2 = dynamic_cast<ExpressionStatement const*>(stmts[1].get());
    auto assign_1 = dynamic_cast<Assignment const*>(&stmt_1->expression());
    auto assign_2 = dynamic_cast<Assignment const*>(&stmt_2->expression());
    auto ctor = dynamic_cast<FunctionCall const*>(&assign_2->rightHandSide());

    auto res_1 = node_to_ref(*ctrt_a);
    auto res_2 = node_to_ref(assign_1->leftHandSide());
    auto res_3 = node_to_ref(ctor->expression());

    BOOST_CHECK_EQUAL(res_1, nullptr);
    BOOST_CHECK_EQUAL(res_2->name(), "a");
    BOOST_CHECK_EQUAL(res_3->name(), "C");
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
