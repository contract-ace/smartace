/**
 * @date 2019
 * Test suite targeting expression conversion. These tests are outside of the
 * context of any statements, allowing for simplification and fine-tuning.
 * 
 * Targets libsolidity/modelcheck/ExpressionConversionVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/ExpressionConverter.h>

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

using SubD = Literal::SubDenomination;

VariableScopeResolver _prime_resolver(shared_ptr<string> name)
{
    using DeclVis = Declaration::Visibility;
    VariableScopeResolver resolver;
    resolver.enter();
    resolver.record_declaration(VariableDeclaration(
        SourceLocation(), nullptr, name, nullptr, DeclVis::Public
    ));
    return resolver;
}

string _convert_assignment(Token tok)
{
    auto id_name = make_shared<string>("a");
    auto id = make_shared<Identifier>(SourceLocation(), id_name);

    auto op = make_shared<BinaryOperation>(
        SourceLocation(), id, Token::BitXor, id
    );

    Assignment assgn(SourceLocation(), id, tok, op);

    ostringstream oss;
    oss << *ExpressionConverter(assgn, {}, _prime_resolver(id_name)).convert();
    return oss.str();
}

string _convert_binop(Token tok)
{
    auto name_a = make_shared<string>("a");
    auto id_a = make_shared<Identifier>(SourceLocation(), name_a);
    auto id_b = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("b")
    );

    BinaryOperation op(SourceLocation(), id_a, tok, id_b);

    ostringstream oss;
    oss << *ExpressionConverter(op, {}, _prime_resolver(name_a)).convert();
    return oss.str();
}

string _convert_unaryop(Token tok, shared_ptr<Expression> expr, bool prefix)
{
    UnaryOperation op(SourceLocation(), tok, expr, prefix);

    ostringstream oss;
    auto resolver = _prime_resolver(make_shared<string>("a"));
    oss << *ExpressionConverter(op, {}, resolver).convert();
    return oss.str();
}

string _convert_literal(Token tok, string src, SubD subdom = SubD::None)
{
    Literal lit(SourceLocation(), tok, make_shared<string>(src), subdom);

    ostringstream oss;
    oss << *ExpressionConverter(lit, {}, {}).convert();
    return oss.str();
}

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(ExpressionConversion)

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

    // Ensur1es that only the first l-value is checked.
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(*m1).find(), nullptr);
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(indx).find(), nullptr);

    // Checks that branches which cannot return l-values are pruned.
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(call).find(), nullptr);
    BOOST_CHECK_EQUAL(LValueSniffer<Identifier>(indx).find(), nullptr);
}

// Ensures that conditional expressions are mapped into C conditionals.
BOOST_AUTO_TEST_CASE(conditional_expression)
{
    auto name_a = make_shared<string>("a");
    auto var_a = make_shared<Identifier>(SourceLocation(), name_a);
    auto var_b = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("b")
    );
    auto var_c = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("c")
    );

    Conditional cond(SourceLocation(), var_a, var_b, var_c);

    ostringstream oss;
    oss << *ExpressionConverter(cond, {}, _prime_resolver(name_a)).convert();
    BOOST_CHECK_EQUAL(oss.str(), "(a)?(self->d_b):(self->d_c)");
}

// Checks the assignment to non-map types is supported. Ensures that when
// assignment is not simple, it is decomposed to simple assignment. This
// decomposition is required in assignment to map types.
BOOST_AUTO_TEST_CASE(assignment_expression)
{
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::Assign), "(a)=((a)^(a))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignBitOr), "(a)=((a)|((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignBitAnd), "(a)=((a)&((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignShl), "(a)=((a)<<((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignAdd), "(a)=((a)+((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignSub), "(a)=((a)-((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignMul), "(a)=((a)*((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignDiv), "(a)=((a)/((a)^(a)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignMod), "(a)=((a)%((a)^(a)))"
    );
    // TODO(scottwe): SAR, SHR
}

// Ensures that tuples of varying sizes are handled correctly.
BOOST_AUTO_TEST_CASE(tuple_expression)
{
    auto var_a = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a")
    );

    TupleExpression one_tuple(SourceLocation(), {var_a}, false);
    // TODO(scottwe): two_tuple
    // TODO(scottwe): three_tuple
    // TODO(scottwe): empty array
    // TODO(scottwe): n element array, large n

    ostringstream oss;
    oss << *ExpressionConverter(one_tuple, {}, {}).convert();
    BOOST_CHECK_EQUAL(oss.str(), "self->d_a");
}

// Ensures that unary expressions map to their corresponding expressions in C.
BOOST_AUTO_TEST_CASE(unary_expression)
{
    auto val = make_shared<Literal>(
        SourceLocation(), Token::FalseLiteral, make_shared<string>("false")
    );
    auto var = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a")
    );

    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Not, val, true), "!(0)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::BitNot, val, true), "!(0)");
    // TODO(scottwe): test Token::Delete.
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Inc, var, true), "++(a)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Dec, var, true), "--(a)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Inc, var, false), "(a)++");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Dec, var, false), "(a)--");
}

// Ensures that binary expressions map to their corresponding expressions in C.
BOOST_AUTO_TEST_CASE(binary_expression)
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
    BOOST_CHECK_EQUAL(_convert_binop(
        Token::LessThanOrEqual), "(a)<=(self->d_b)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::GreaterThanOrEqual), "(a)>=(self->d_b)"
    );
}

// Ensures that identifiers are resolved, using the current scope.
BOOST_AUTO_TEST_CASE(identifier_expression)
{
    auto name_a = make_shared<string>("a");
    Identifier id_a(SourceLocation(), name_a);
    Identifier id_b(SourceLocation(), make_shared<string>("b"));
    Identifier msg(SourceLocation(), make_shared<string>("msg"));

    auto resolver = _prime_resolver(name_a);

    ostringstream a_oss, b_oss, msg_oss;
    a_oss << *ExpressionConverter(id_a, {}, resolver).convert();
    b_oss << *ExpressionConverter(id_b, {}, resolver).convert();
    msg_oss << *ExpressionConverter(msg, {}, resolver).convert();

    BOOST_CHECK_EQUAL(a_oss.str(), "a");
    BOOST_CHECK_EQUAL(b_oss.str(), "self->d_b");
    BOOST_CHECK_EQUAL(msg_oss.str(), "state");
}

// Ensures all literal types are converted in the expected way, and that all
// valid sub-denominations are taken into account while encoding said literal.
BOOST_AUTO_TEST_CASE(literal_expression)
{
    BOOST_CHECK_EQUAL(_convert_literal(Token::TrueLiteral, "true"), "1");
    BOOST_CHECK_EQUAL(_convert_literal(Token::FalseLiteral, "false"), "0");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::StringLiteral, "string"),
        to_string((long long int)(hash<string>()("string")))
    );
    BOOST_CHECK_EQUAL(_convert_literal(Token::Number, "432"), "432");
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "432 seconds", SubD::Second), "432"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "432 wei", SubD::Wei), "432"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 minutes", SubD::Minute), "120"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 hours", SubD::Hour), "7200"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 days", SubD::Day), "172800"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 weeks", SubD::Week), "1209600"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 years", SubD::Year), "63072000"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 szabo", SubD::Szabo), "2000000000000"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 finney", SubD::Finney),
        "2000000000000000"
    );
    BOOST_CHECK_EQUAL(
        _convert_literal(Token::Number, "2 ether", SubD::Ether),
        "2000000000000000000"
    );
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
