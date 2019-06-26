/**
 * @date 2019
 * Performs a call-by-call test of the FunctionDefinitionGenerator, to ensure
 * that parameters are populated correctly.
 */

#include <libsolidity/modelcheck/FunctionDefinitionGenerator.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(FunctionDefinitionGeneration, ::dev::solidity::test::AnalysisFramework)

BOOST_AUTO_TEST_CASE(default_function)
{
    char const* text = "contract A {}";

    SourceUnit const* ast = parseAndAnalyse(text);

    const FunctionDefinitionGenerator gen(*ast, false);
    ASTPointer<FunctionDefinition> func = gen.generate();

    BOOST_CHECK_EQUAL(func->location(), ast->location());
    BOOST_CHECK(!func->isConstructor());
    BOOST_CHECK(func->parameterList().parameters().empty());
    BOOST_CHECK(func->returnParameterList()->parameters().empty());
}

BOOST_AUTO_TEST_CASE(default_ctor)
{
    char const* text = "contract A {}";

    SourceUnit const* ast = parseAndAnalyse(text);

    const FunctionDefinitionGenerator gen(*ast, true);
    ASTPointer<FunctionDefinition> ctor = gen.generate();

    BOOST_CHECK_EQUAL(ctor->location(), ast->location());
    BOOST_CHECK(ctor->isConstructor());
    BOOST_CHECK(ctor->parameters().empty());
    BOOST_CHECK(ctor->returnParameters().empty());
}

BOOST_AUTO_TEST_CASE(add_params)
{
    char const* text = "contract A {}";

    SourceUnit const* ast = parseAndAnalyse(text);

    FunctionDefinitionGenerator gen(*ast, true);
    
    vector<ASTPointer<VariableDeclaration>> var_ptrs;
    for (unsigned int i = 0; i < 5; ++i)
    {
        auto var_ptr = make_shared<VariableDeclaration>(
            ASTNode::SourceLocation(),
            nullptr,
            nullptr,
            nullptr,
            Declaration::Visibility::Default
        );
        var_ptrs.push_back(var_ptr);
    }

    unsigned int ct = 0;
    for (const auto var : var_ptrs)
    {
        gen.push_arg(var);
        gen.push_retval(var);
        ++ct;

        ASTPointer<FunctionDefinition> f = gen.generate();
        BOOST_CHECK_EQUAL(f->parameters().size(), ct);
        BOOST_CHECK_EQUAL(f->returnParameters().size(), ct);

        for (unsigned int i = 0; i < ct; ++i)
        {
            BOOST_CHECK_EQUAL(f->parameters()[i], var_ptrs[i]);
            BOOST_CHECK_EQUAL(f->returnParameters()[i], var_ptrs[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(add_statements)
{
    char const* text = "contract A {}";

    SourceUnit const* ast = parseAndAnalyse(text);

    FunctionDefinitionGenerator gen(*ast, true);

    vector<ASTPointer<Statement>> stmt_ptrs;
    for (unsigned int i = 0; i < 5; ++i)
    {
        auto stmt_ptr = make_shared<PlaceholderStatement>(
            ASTNode::SourceLocation(), nullptr
        );
        stmt_ptrs.push_back(stmt_ptr);
    }

    unsigned int ct = 0;
    for (const auto var : stmt_ptrs)
    {
        gen.push_statement(var);
        ++ct;

        ASTPointer<FunctionDefinition> f = gen.generate();
        BOOST_CHECK_EQUAL(f->body().statements().size(), ct);

        for (unsigned int i = 0; i < ct; ++i)
        {
            BOOST_CHECK_EQUAL(f->body().statements()[i], stmt_ptrs[i]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
