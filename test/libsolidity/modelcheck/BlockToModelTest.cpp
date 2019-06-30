/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/BlockToModelTest.{h,cpp}.
 */


#include <libsolidity/modelcheck/BlockToModelVisitor.h>

#include <test/libsolidity/AnalysisFramework.h>
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

BOOST_FIXTURE_TEST_SUITE(BlockToModel, ::dev::solidity::test::AnalysisFramework)

BOOST_AUTO_TEST_CASE(decl_statement)
{
    char const* text = R"(
		contract A {
            struct B { int a; }
			function f() public {
                int i1;
                int i2 = 10;
                B memory b;
            }
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &body = ctrt.definedFunctions()[0]->body();

    TypeTranslator translator;
    translator.enter_scope(ctrt);

    ostringstream actual;
    BlockToModelVisitor visitor(body, translator);
    visitor.print(actual);

    ostringstream expected;
    expected << "int i1;" << endl
             << "int i2 = 10;" << endl
             << "struct A_B b;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

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

BOOST_AUTO_TEST_CASE(unary_op)
{
    auto literal = make_shared<Literal>(
        SourceLocation(), langutil::Token::Number, make_shared<string>("1"));

    auto not_op = make_shared<UnaryOperation>(
        SourceLocation(), langutil::Token::Not, literal, true);
    auto bnot = make_shared<UnaryOperation>(
        SourceLocation(), langutil::Token::BitNot, literal, true);
    // TODO(scottwe):
    // auto del_op = make_shared<UnaryOperation>(
    //     SourceLocation(), langutil::Token::Delete, literal, true);
    auto pri_op = make_shared<UnaryOperation>(
        SourceLocation(), langutil::Token::Inc, literal, true);
    auto prd_op = make_shared<UnaryOperation>(
        SourceLocation(), langutil::Token::Dec, literal, true);
    auto poi_op = make_shared<UnaryOperation>(
        SourceLocation(), langutil::Token::Inc, literal, false);
    auto pod_op = make_shared<UnaryOperation>(
        SourceLocation(), langutil::Token::Dec, literal, false);

    Block block(SourceLocation(), nullptr, {
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, not_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, bnot),
        // TODO(scottwe):
        // make_shared<ExpressionStatement>(SourceLocation(), nullptr, del_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, pri_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, prd_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, poi_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, pod_op)});

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "!(1);" << endl;
    expected << "!(1);" << endl;
    // TODO(scottwe): delete test
    expected << "++(1);" << endl;
    expected << "--(1);" << endl;
    expected << "(1)++;" << endl;
    expected << "(1)--;" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(binary_op)
{
    auto lit_1 = make_shared<Literal>(
        SourceLocation(), langutil::Token::Number, make_shared<string>("1"));
    auto lit_2 = make_shared<Literal>(
        SourceLocation(), langutil::Token::Number, make_shared<string>("2"));

    auto cma_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Comma, lit_2);
    auto or_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Or, lit_2);
    auto and_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::And, lit_2);
    auto bor_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::BitOr, lit_2);
    auto bxor_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::BitXor, lit_2);
    auto band_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::BitAnd, lit_2);
    auto shl_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::SHL, lit_2);
    // TODO(scottwe):
    // auto sar_op = make_shared<BinaryOperation>(
    //     SourceLocation(), lit_1, Token::SAR, lit_2);
    // TODO(scottwe):
    // auto shr_op = make_shared<BinaryOperation>(
    //     SourceLocation(), lit_1, Token::SHR, lit_2);
    auto add_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Add, lit_2);
    auto sub_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Sub, lit_2);
    auto mul_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Mul, lit_2);
    auto div_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Div, lit_2);
    auto mod_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Mod, lit_2);
    // TODO(scottwe):
    // auto exp_op = make_shared<BinaryOperation>(
    //     SourceLocation(), lit_1, Token::Exp, lit_2);
    auto eq_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::Equal, lit_2);
    auto neq_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::NotEqual, lit_2);
    auto lt_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::LessThan, lit_2);
    auto gt_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::GreaterThan, lit_2);
    auto lte_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::LessThanOrEqual, lit_2);
    auto gte_op = make_shared<BinaryOperation>(
        SourceLocation(), lit_1, Token::GreaterThanOrEqual, lit_2);

    Block block(SourceLocation(), nullptr, {
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, cma_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, or_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, and_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, bor_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, bxor_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, band_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, shl_op),
        // TODO(scottwe):
        // make_shared<ExpressionStatement>(SourceLocation(), nullptr, sar_op),
        // TODO(scottwe):
        // make_shared<ExpressionStatement>(SourceLocation(), nullptr, shr_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, add_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, sub_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, mul_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, div_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, mod_op),
        // TODO(scottwe):
        // make_shared<ExpressionStatement>(SourceLocation(), nullptr, exp_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, eq_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, neq_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, lt_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, gt_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, lte_op),
        make_shared<ExpressionStatement>(SourceLocation(), nullptr, gte_op)});

    ostringstream actual;
    BlockToModelVisitor visitor(block, TypeTranslator());
    visitor.print(actual);

    ostringstream expected;
    expected << "(1),(2);" << endl;
    expected << "(1)||(2);" << endl;
    expected << "(1)&&(2);" << endl;
    expected << "(1)|(2);" << endl;
    expected << "(1)^(2);" << endl;
    expected << "(1)&(2);" << endl;
    expected << "(1)<<(2);" << endl;
    // TODO(scottwe): sar test
    // TODO(scottwe): shr test
    expected << "(1)+(2);" << endl;
    expected << "(1)-(2);" << endl;
    expected << "(1)*(2);" << endl;
    expected << "(1)/(2);" << endl;
    expected << "(1)%(2);" << endl;
    // TODO(scottwe): exp test
    expected << "(1)==(2);" << endl;
    expected << "(1)!=(2);" << endl;
    expected << "(1)<(2);" << endl;
    expected << "(1)>(2);" << endl;
    expected << "(1)<=(2);" << endl;
    expected << "(1)>=(2);" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_CASE(var_id)
{
    char const* text = R"(
		contract A {
            int a;
            int c;
			function f() public returns (int) {
                int b;
                if (a == 1) {
                    int c;
                    ++a;
                    ++b;
                    ++c;
                } else {
                    ++a;
                    ++b;
                    ++c;
                }
                ++a;
                ++b;
                ++c;
            }
		}
	)";

    const auto &unit = *parseAndAnalyse(text);
    const auto &ctrt = *retrieveContractByName(unit, "A");
    const auto &body = ctrt.definedFunctions()[0]->body();

    TypeTranslator translator;
    translator.enter_scope(ctrt);

    ostringstream actual;
    BlockToModelVisitor visitor(body, translator);
    visitor.print(actual);

    ostringstream expected;
    expected << "int b;" << endl
             << "if ((self->d_a)==(1)) {" << endl
             << "int c;" << endl
             << "++(self->d_a);" << endl
             << "++(b);" << endl
             << "++(c);" << endl
             << "} else {" << endl
             << "++(self->d_a);" << endl
             << "++(b);" << endl
             << "++(self->d_c);" << endl
             << "}" << endl
             << "++(self->d_a);" << endl
             << "++(b);" << endl
             << "++(self->d_c);" << endl;

    BOOST_CHECK_EQUAL(actual.str(), expected.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
