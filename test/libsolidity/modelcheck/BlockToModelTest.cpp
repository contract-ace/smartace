/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/BlockToModelTest.{h,cpp}.
 */


#include <libsolidity/modelcheck/BlockToModelVisitor.h>

#include <boost/test/unit_test.hpp>
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

BOOST_AUTO_TEST_SUITE(BlockToModel);

BOOST_AUTO_TEST_CASE(if_statement)
{
    auto true_strnptr = make_shared<string>("true");
    auto ture_literal = make_shared<Literal>(
        langutil::SourceLocation(), langutil::Token::TrueLiteral, true_strnptr);

    auto null_stmt = make_shared<PlaceholderStatement>(
        langutil::SourceLocation(), nullptr);

    vector<ASTPointer<Statement>> stmts;
    stmts.push_back(make_shared<IfStatement>(
        langutil::SourceLocation(), nullptr, ture_literal, null_stmt, nullptr));
    stmts.push_back(make_shared<IfStatement>(
        langutil::SourceLocation(), nullptr, ture_literal, null_stmt, null_stmt));

    Block block(langutil::SourceLocation(), nullptr, stmts);

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "if (1) {" << endl
             << "}" << endl
             << "if (1) {" << endl
             << "} else {" << endl
             << "}" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(literal_expression_statement)
{
    auto true_strnptr = make_shared<string>("true");
    auto true_literal = make_shared<Literal>(
        langutil::SourceLocation(), langutil::Token::TrueLiteral, true_strnptr);

    auto false_strnptr = make_shared<string>("false");
    auto false_literal = make_shared<Literal>(
        langutil::SourceLocation(), langutil::Token::FalseLiteral, false_strnptr);

    auto strn_strnptr = make_shared<string>("string");
    auto strn_literal = make_shared<Literal>(
        langutil::SourceLocation(), langutil::Token::StringLiteral, strn_strnptr);

    auto numb_strnptr = make_shared<string>("432");
    auto numb_literal = make_shared<Literal>(
        langutil::SourceLocation(), langutil::Token::Number, numb_strnptr);

    vector<ASTPointer<Statement>> stmts{
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, true_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, false_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, strn_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, numb_literal
        )
    };

    Block block(langutil::SourceLocation(), nullptr, stmts);

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "1;" << endl
             << "0;" << endl
             << hash<string>()(*strn_strnptr) << ";" << endl
             << "432;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
