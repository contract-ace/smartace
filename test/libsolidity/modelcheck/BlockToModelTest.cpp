/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/BlockToModelTest.{h,cpp}.
 */


#include <libsolidity/modelcheck/BlockToModelVisitor.h>

#include <boost/test/unit_test.hpp>
#include <sstream>

using namespace std;
using langutil::SourceLocation;

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
        SourceLocation(), langutil::Token::TrueLiteral, true_strnptr);

    auto null_stmt = make_shared<PlaceholderStatement>(
        SourceLocation(), nullptr);

    vector<ASTPointer<Statement>> stmts;
    stmts.push_back(make_shared<IfStatement>(
        SourceLocation(), nullptr, ture_literal, null_stmt, nullptr));
    stmts.push_back(make_shared<IfStatement>(
        SourceLocation(), nullptr, ture_literal, null_stmt, null_stmt));

    Block block(SourceLocation(), nullptr, stmts);

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
    using SubDom = Literal::SubDenomination;
    constexpr langutil::Token Number = langutil::Token::Number;

    auto true_strnptr = make_shared<string>("true");
    auto true_literal = make_shared<Literal>(
        SourceLocation(), langutil::Token::TrueLiteral, true_strnptr);

    auto false_strnptr = make_shared<string>("false");
    auto false_literal = make_shared<Literal>(
        SourceLocation(), langutil::Token::FalseLiteral, false_strnptr);

    auto strn_strnptr = make_shared<string>("string");
    auto strn_literal = make_shared<Literal>(
        SourceLocation(), langutil::Token::StringLiteral, strn_strnptr);

    auto numb_strnptr = make_shared<string>("432");
    auto numb_literal = make_shared<Literal>(
        SourceLocation(), Number, numb_strnptr);

    auto sec_strnptr = make_shared<string>("432 seconds");
    auto sec_literal = make_shared<Literal>(
        SourceLocation(), Number, sec_strnptr, SubDom::Second);

    auto wei_strnptr = make_shared<string>("432 wei");
    auto wei_literal = make_shared<Literal>(
        SourceLocation(), Number, wei_strnptr, SubDom::Wei);

    auto min_strnptr = make_shared<string>("2 minutes");
    auto min_literal = make_shared<Literal>(
        SourceLocation(), Number, min_strnptr, SubDom::Minute);

    auto hr_strnptr = make_shared<string>("2 hours");
    auto hr_literal = make_shared<Literal>(
        SourceLocation(), Number, hr_strnptr, SubDom::Hour);

    auto day_strnptr = make_shared<string>("2 days");
    auto day_literal = make_shared<Literal>(
        SourceLocation(), Number, day_strnptr, SubDom::Day);

    auto week_strnptr = make_shared<string>("2 weeks");
    auto week_literal = make_shared<Literal>(
        SourceLocation(), Number, week_strnptr, SubDom::Week);

    auto year_strnptr = make_shared<string>("2 years");
    auto year_literal = make_shared<Literal>(
        SourceLocation(), Number, year_strnptr, SubDom::Year);

    auto sz_strnptr = make_shared<string>("2 szabo");
    auto sz_literal = make_shared<Literal>(
        SourceLocation(), Number, sz_strnptr, SubDom::Szabo);

    auto fin_strnptr = make_shared<string>("2 finney");
    auto fin_literal = make_shared<Literal>(
        SourceLocation(), Number, fin_strnptr, SubDom::Finney);

    auto eth_strnptr = make_shared<string>("2 ether");
    auto eth_literal = make_shared<Literal>(
        SourceLocation(), Number, eth_strnptr, SubDom::Ether);

    vector<ASTPointer<Statement>> stmts{
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, true_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, false_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, strn_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, numb_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, sec_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, wei_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, min_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, hr_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, day_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, week_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, year_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, sz_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, fin_literal
        ),
        make_shared<ExpressionStatement>(
            SourceLocation(), nullptr, eth_literal
        )
    };

    Block block(SourceLocation(), nullptr, stmts);

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "1;" << endl
             << "0;" << endl
             << hash<string>()(*strn_strnptr) << ";" << endl
             << "432;" << endl
             << "432;" << endl
             << "432;" << endl
             << "120;" << endl
             << "7200;" << endl
             << "172800;" << endl
             << "1209600;" << endl
             << "63072000;" << endl
             << "2000000000000;" << endl
             << "2000000000000000;" << endl
             << "2000000000000000000;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(return_statement)
{
    auto str = make_shared<string>("432");
    auto lit = make_shared<Literal>(SourceLocation(), langutil::Token::Number, str);

    vector<ASTPointer<Statement>> stmts{
        make_shared<Return>(SourceLocation(), nullptr, nullptr),
        make_shared<Return>(SourceLocation(), nullptr, lit)
    };

    Block block(SourceLocation(), nullptr, stmts);

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "return;" << endl
             << "return 432;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(break_statement)
{
    Block block(
        SourceLocation(), nullptr, {make_shared<Break>(SourceLocation(), nullptr)});

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "break;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(continue_statement)
{
    Block block(
        SourceLocation(), nullptr, {make_shared<Continue>(SourceLocation(), nullptr)});

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "continue;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
