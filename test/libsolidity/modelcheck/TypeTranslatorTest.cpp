/*
 * @date 2019
 * A partial test suite to validate key features in the type translation unit.
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>
#include <libsolidity/analysis/GlobalContext.h>
#include <sstream>

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

BOOST_FIXTURE_TEST_SUITE(TypeTranslations, ::dev::solidity::test::AnalysisFramework)

// Integration test which should fail if a new member is added to the global
// context, and that variable is not added to the type translator's lookup. This
// lookup is m_global_context. Success occurs when no exceptions are thrown.
//
// This test works by building a source unit with a single contract, consisting
// of a single function, consisting of expression statements using each global
// context identifier. The resulting contract may not be "valid" due to the use
// of MagicTypes.
BOOST_AUTO_TEST_CASE(global_context_ids)
{
    vector<ASTPointer<Statement>> stmts;
    vector<ASTPointer<VariableDeclaration>> params;
    vector<ASTPointer<ModifierInvocation>> invocs;
    vector<ASTPointer<InheritanceSpecifier>> parents;

    GlobalContext ctx;
    auto decls = ctx.declarations();
    for (auto decl : decls)
    {
        auto name = make_shared<string>(decl->name());
        auto stmt = make_shared<ExpressionStatement>(
            SourceLocation(),
            nullptr,
            make_shared<Identifier>(SourceLocation(), name));

        stmts.push_back(stmt);
    }
    BOOST_CHECK_EQUAL(decls.size(), stmts.size());

    auto func = make_shared<FunctionDefinition>(
        SourceLocation(),
        make_shared<string>("f"),
        Declaration::Visibility::Default,
        StateMutability::NonPayable,
        false,
        nullptr,
        make_shared<ParameterList>(SourceLocation(), params),
        invocs,
        make_shared<ParameterList>(SourceLocation(), params),
        make_shared<Block>(SourceLocation(), nullptr, stmts));
    BOOST_CHECK_EQUAL(func->body().statements().size(), stmts.size());

    auto contract = make_shared<ContractDefinition>(
        SourceLocation(),
        make_shared<string>("A"),
        nullptr,
        parents,
        vector<ASTPointer<ASTNode>>{func});
    BOOST_CHECK_EQUAL(contract->definedFunctions().size(), 1);

    SourceUnit unit(SourceLocation(), {contract});
    BOOST_CHECK_EQUAL(unit.nodes().size(), 1);

    TypeConverter converter;
    converter.record(unit);
}

BOOST_AUTO_TEST_CASE(this_keyword_ids)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IdenExprPtr = Identifier const*;

    char const* text = R"(
        contract A {
            function f() public {
                this;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(ast, "A");
    auto const &func = *ctrt.definedFunctions()[0];

    auto const &stmt = *func.body().statements()[0];
    auto const &expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const &iden = *dynamic_cast<IdenExprPtr>(&expr);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(iden).name, "A");
}

BOOST_AUTO_TEST_CASE(super_keyword_ids)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IdenExprPtr = Identifier const*;

    char const* text = R"(
        contract A {
            function f() public {
                this;
            }
        }
    )";

    auto const &ast = *parseAndAnalyse(text);
    auto const &ctrt = *retrieveContractByName(ast, "A");
    auto const &func = *ctrt.definedFunctions()[0];

    auto const &stmt = *func.body().statements()[0];
    auto const &expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const &iden = *dynamic_cast<IdenExprPtr>(&expr);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(iden).name, "A");
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
