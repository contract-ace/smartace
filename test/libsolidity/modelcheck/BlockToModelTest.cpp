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
    using SubDom = Literal::SubDenomination;
    constexpr langutil::Token Number = langutil::Token::Number;

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
        langutil::SourceLocation(), Number, numb_strnptr);

    auto sec_strnptr = make_shared<string>("432 seconds");
    auto sec_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, sec_strnptr, SubDom::Second);

    auto wei_strnptr = make_shared<string>("432 wei");
    auto wei_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, wei_strnptr, SubDom::Wei);

    auto min_strnptr = make_shared<string>("2 minutes");
    auto min_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, min_strnptr, SubDom::Minute);

    auto hr_strnptr = make_shared<string>("2 hours");
    auto hr_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, hr_strnptr, SubDom::Hour);

    auto day_strnptr = make_shared<string>("2 days");
    auto day_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, day_strnptr, SubDom::Day);

    auto week_strnptr = make_shared<string>("2 weeks");
    auto week_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, week_strnptr, SubDom::Week);

    auto year_strnptr = make_shared<string>("2 years");
    auto year_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, year_strnptr, SubDom::Year);

    auto sz_strnptr = make_shared<string>("2 szabo");
    auto sz_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, sz_strnptr, SubDom::Szabo);

    auto fin_strnptr = make_shared<string>("2 finney");
    auto fin_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, fin_strnptr, SubDom::Finney);

    auto eth_strnptr = make_shared<string>("2 ether");
    auto eth_literal = make_shared<Literal>(
        langutil::SourceLocation(), Number, eth_strnptr, SubDom::Ether);


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
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, sec_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, wei_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, min_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, hr_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, day_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, week_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, year_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, sz_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, fin_literal
        ),
        make_shared<ExpressionStatement>(
            langutil::SourceLocation(), nullptr, eth_literal
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

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
