/*
 * Test suite targeting expression conversion. These tests are outside of the
 * context of any statements, allowing for simplification and fine-tuning.
 * 
 * Targets libsolidity/modelcheck/ExpressionConversionVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/ExpressionConversionVisitor.h>

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <vector>

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

string _convert_assignment(Token tok)
{
    const TypeTranslator translator;
    VariableScopeResolver resolver;

    auto id_name = make_shared<string>("a");
    auto id = make_shared<Identifier>(SourceLocation(), id_name);

    resolver.enter();
    resolver.record_declaration(VariableDeclaration(
        SourceLocation(),
        nullptr,
        id_name,
        nullptr,
        Declaration::Visibility::Public));

    auto op = make_shared<BinaryOperation>(
        SourceLocation(), id, Token::BitXor, id);

    Assignment assignment(SourceLocation(), id, tok, op);

    ostringstream oss;
    ExpressionConversionVisitor visitor(assignment, translator, resolver);
    visitor.print(oss);

    return oss.str();
}

string _convert_binop(Token tok)
{
    const TypeTranslator translator;
    VariableScopeResolver resolver;

    resolver.enter();
    resolver.record_declaration(VariableDeclaration(
        SourceLocation(),
        nullptr,
        make_shared<string>("a"),
        nullptr,
        Declaration::Visibility::Public));

    auto id_a = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a"));
    auto id_b = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("b"));

    BinaryOperation op(SourceLocation(), id_a, tok, id_b);

    ostringstream oss;
    ExpressionConversionVisitor visitor(op, translator, resolver);
    visitor.print(oss);

    return oss.str();
}

string _convert_unaryop(Token tok, shared_ptr<Expression> expr, bool prefix)
{
    const TypeTranslator translator;
    VariableScopeResolver resolver;

    resolver.enter();
    resolver.record_declaration(VariableDeclaration(
        SourceLocation(),
        nullptr,
        make_shared<string>("a"),
        nullptr,
        Declaration::Visibility::Public));

    UnaryOperation op(SourceLocation(), tok, expr, prefix);
    ExpressionConversionVisitor visitor(op, translator, resolver);

    ostringstream oss;
    visitor.print(oss);

    return oss.str();
}

string _convert_literal(
    Token tok,
    string src,
    Literal::SubDenomination subdom = Literal::SubDenomination::None)
{
    const TypeTranslator translator;
    const VariableScopeResolver resolver;

    Literal lit(SourceLocation(), tok, make_shared<string>(src), subdom);
    ExpressionConversionVisitor visitor(lit, translator, resolver);

    ostringstream oss;
    visitor.print(oss);

    return oss.str();
}

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(ExpressionConversion)

BOOST_AUTO_TEST_CASE(member_access_sniffer)
{
    auto id = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a"));

    auto m1 = make_shared<MemberAccess>(SourceLocation(), id, nullptr);
    auto m2 = make_shared<MemberAccess>(SourceLocation(), m1, nullptr);

    TupleExpression tuple(SourceLocation(), {m2}, false);

    MemberAccessSniffer v1(tuple);
    MemberAccessSniffer v2(*m2);
    MemberAccessSniffer v3(*m1);
    MemberAccessSniffer v4(*id);

    BOOST_CHECK_EQUAL(v1.find(), m2.get());
    BOOST_CHECK_EQUAL(v2.find(), m2.get());
    BOOST_CHECK_EQUAL(v3.find(), m1.get());
    BOOST_CHECK_EQUAL(v4.find(), nullptr);
}

BOOST_AUTO_TEST_CASE(conditional_expression_output)
{
    auto var_a = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a"));
    auto var_b = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("b"));
    auto var_c = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("c"));

    Conditional cond(SourceLocation(), var_a, var_b, var_c);

    const TypeTranslator translator;
    VariableScopeResolver resolver;

    resolver.enter();
    resolver.record_declaration(VariableDeclaration(
        SourceLocation(),
        nullptr,
        make_shared<string>("a"),
        nullptr,
        Declaration::Visibility::Public));

    ExpressionConversionVisitor visitor(cond, translator, resolver);

    ostringstream oss;
    visitor.print(oss);
    BOOST_CHECK_EQUAL(oss.str(), "(a)?(self->d_b):(self->d_c)");
}

BOOST_AUTO_TEST_CASE(assignment_expression_output)
{
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::Assign), "(a)=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignBitOr), "(a)|=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignBitAnd), "(a)&=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignShl), "(a)<<=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignAdd), "(a)+=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignSub), "(a)-=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignMul), "(a)*=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignDiv), "(a)/=((a)^(a))");
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignMod), "(a)%=((a)^(a))");
}

BOOST_AUTO_TEST_CASE(tuple_expression_output)
{
    auto var_a = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a"));

    TupleExpression one_tuple(SourceLocation(), {var_a}, false);
    // TODO(scottwe): two_tuple
    // TODO(scottwe): three_tuple
    // TODO(scottwe): empty array
    // TODO(scottwe): n element array, large n

    const TypeTranslator translator;
    const VariableScopeResolver resolver;

    ExpressionConversionVisitor visitor(one_tuple, translator, resolver);

    ostringstream oss;
    visitor.print(oss);
    BOOST_CHECK_EQUAL(oss.str(), "self->d_a");
}

BOOST_AUTO_TEST_CASE(unary_expression_output)
{
    auto val = make_shared<Literal>(
        SourceLocation(), Token::FalseLiteral, make_shared<string>("false"));
    auto var = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a"));

    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Not, val, true), "!(0)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::BitNot, val, true), "!(0)");
    // TODO(scottwe): test Token::Delete.
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Inc, var, true), "++(a)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Dec, var, true), "--(a)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Inc, var, false), "(a)++");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Dec, var, false), "(a)--");
}

BOOST_AUTO_TEST_CASE(binary_expression_output)
{
    BOOST_CHECK_EQUAL(_convert_binop(Token::Comma), "(a),(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::Or), "(a)||(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::And), "(a)&&(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::BitOr), "(a)|(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::BitXor), "(a)^(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::BitAnd), "(a)&(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::SHL), "(a)<<(self->d_b)");
    // TODO(scottwe): Token::SAR test
    // TODO(scottwe): Token::SHR test
    BOOST_CHECK_EQUAL(_convert_binop(Token::Add), "(a)+(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::Sub), "(a)-(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::Mul), "(a)*(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::Div), "(a)/(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::Mod), "(a)%(self->d_b)");
    // TODO(scottwe): Token::EXP test
    BOOST_CHECK_EQUAL(_convert_binop(Token::Equal), "(a)==(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::NotEqual), "(a)!=(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::LessThan), "(a)<(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::GreaterThan), "(a)>(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::LessThanOrEqual), "(a)<=(self->d_b)");
    BOOST_CHECK_EQUAL(_convert_binop(Token::GreaterThanOrEqual), "(a)>=(self->d_b)");
}

BOOST_AUTO_TEST_CASE(identifier_expression_output)
{
    Identifier id_a(SourceLocation(), make_shared<string>("a"));
    Identifier id_b(SourceLocation(), make_shared<string>("b"));
    Identifier msg(SourceLocation(), make_shared<string>("msg"));

    const TypeTranslator translator;
    VariableScopeResolver resolver;

    resolver.enter();
    resolver.record_declaration(VariableDeclaration(
        SourceLocation(),
        nullptr,
        make_shared<string>("a"),
        nullptr,
        Declaration::Visibility::Public));

    ExpressionConversionVisitor a_visitor(id_a, translator, resolver);
    ExpressionConversionVisitor b_visitor(id_b, translator, resolver);
    ExpressionConversionVisitor msg_visitor(msg, translator, resolver);

    ostringstream a_oss, b_oss, msg_oss;
    a_visitor.print(a_oss);
    b_visitor.print(b_oss);
    msg_visitor.print(msg_oss);

    BOOST_CHECK_EQUAL(a_oss.str(), "a");
    BOOST_CHECK_EQUAL(b_oss.str(), "self->d_b");
    BOOST_CHECK_EQUAL(msg_oss.str(), "state");
}

BOOST_AUTO_TEST_CASE(literal_expression_output)
{
    using SubDom = Literal::SubDenomination;

    BOOST_CHECK_EQUAL(_convert_literal(Token::TrueLiteral, "true"), "1");
    BOOST_CHECK_EQUAL(_convert_literal(Token::FalseLiteral, "false"), "0");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::StringLiteral, "string"),
        to_string(hash<string>()("string")));
    BOOST_CHECK_EQUAL(_convert_literal(Token::Number, "432"), "432");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "432 seconds", SubDom::Second),
        "432");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "432 wei", SubDom::Wei),
        "432");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 minutes", SubDom::Minute),
        "120");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 hours", SubDom::Hour),
        "7200");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 days", SubDom::Day),
        "172800");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 weeks", SubDom::Week),
        "1209600");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 years", SubDom::Year),
        "63072000");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 szabo", SubDom::Szabo),
        "2000000000000");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 finney", SubDom::Finney),
        "2000000000000000");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 ether", SubDom::Ether),
        "2000000000000000000");
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
