/**
 * @date 2019
 * Test suite targeting expression conversion. These tests are outside of the
 * context of any statements, allowing for simplification and fine-tuning.
 * 
 * Targets libsolidity/modelcheck/ExpressionConversionVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/translation/Expression.h>

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

    id->annotation().type = new IntegerType(32);

    auto op = make_shared<BinaryOperation>(
        SourceLocation(), id, Token::BitXor, id
    );

    Assignment assgn(SourceLocation(), id, tok, op);

    ostringstream oss;
    oss << *ExpressionConverter(assgn, {}, {}, _prime_resolver(id_name)).convert();
    return oss.str();
}

string _convert_binop(Token tok)
{
    auto name_a = make_shared<string>("a");
    auto id_a = make_shared<Identifier>(SourceLocation(), name_a);
    auto id_b = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("b")
    );

    id_a->annotation().type = new IntegerType(32);
    id_b->annotation().type = new IntegerType(32);

    BinaryOperation op(SourceLocation(), id_a, tok, id_b);

    ostringstream oss;
    oss << *ExpressionConverter(op, {}, {}, _prime_resolver(name_a)).convert();
    return oss.str();
}

string _convert_unaryop(Token tok, shared_ptr<Expression> expr, bool prefix)
{
    UnaryOperation op(SourceLocation(), tok, expr, prefix);

    ostringstream oss;
    auto resolver = _prime_resolver(make_shared<string>("a"));
    oss << *ExpressionConverter(op, {}, {}, resolver).convert();
    return oss.str();
}

string _convert_literal(Token tok, string src, SubD subdom = SubD::None)
{
    Literal lit(SourceLocation(), tok, make_shared<string>(src), subdom);

    ostringstream oss;
    oss << *ExpressionConverter(lit, {}, {}, {}).convert();
    return oss.str();
}

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(ExpressionConversion)

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

    var_a->annotation().type = new BoolType;
    var_b->annotation().type = new IntegerType(32);
    var_c->annotation().type = new IntegerType(32);

    Conditional cond(SourceLocation(), var_a, var_b, var_c);

    ostringstream oss;
    oss << *ExpressionConverter(cond, {}, {}, _prime_resolver(name_a)).convert();
    BOOST_CHECK_EQUAL(
        oss.str(), "((func_user_a).v)?((self->user_b).v):((self->user_c).v)"
    );
}

// Checks the assignment to non-map types is supported. Ensures that when
// assignment is not simple, it is decomposed to simple assignment. This
// decomposition is required in assignment to map types.
BOOST_AUTO_TEST_CASE(assignment_expression)
{
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::Assign),
        "((func_user_a).v)=(((func_user_a).v)^((func_user_a).v))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignBitOr),
        "((func_user_a).v)=(((func_user_a).v)|(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignBitAnd),
        "((func_user_a).v)=(((func_user_a).v)&(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignShl),
        "((func_user_a).v)=(((func_user_a).v)<<(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignAdd),
        "((func_user_a).v)=(((func_user_a).v)+(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignSub),
        "((func_user_a).v)=(((func_user_a).v)-(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignMul),
        "((func_user_a).v)=(((func_user_a).v)*(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignDiv),
        "((func_user_a).v)=(((func_user_a).v)/(((func_user_a).v)^((func_user_a).v)))"
    );
    BOOST_CHECK_EQUAL(
        _convert_assignment(Token::AssignMod),
        "((func_user_a).v)=(((func_user_a).v)%(((func_user_a).v)^((func_user_a).v)))"
    );
    // TODO(scottwe): SAR, SHR
}

// Ensures that tuples of varying sizes are handled correctly.
BOOST_AUTO_TEST_CASE(tuple_expression)
{
    auto var_a = make_shared<Identifier>(
        SourceLocation(), make_shared<string>("a")
    );
    var_a->annotation().type = new IntegerType(32);

    TupleExpression one_tuple(SourceLocation(), {var_a}, false);
    // TODO(scottwe): two_tuple
    // TODO(scottwe): three_tuple
    // TODO(scottwe): empty array
    // TODO(scottwe): n element array, large n

    ostringstream oss;
    oss << *ExpressionConverter(one_tuple, {}, {}, {}).convert();
    BOOST_CHECK_EQUAL(oss.str(), "(self->user_a).v");
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
    var->annotation().type = new IntegerType(32);

    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Not, val, true), "!(0)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::BitNot, val, true), "~(0)");
    // TODO(scottwe): test Token::Delete.
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Inc, var, true), "++((func_user_a).v)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Dec, var, true), "--((func_user_a).v)");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Inc, var, false), "((func_user_a).v)++");
    BOOST_CHECK_EQUAL(_convert_unaryop(Token::Dec, var, false), "((func_user_a).v)--");
}

// Ensures that binary expressions map to their corresponding expressions in C.
BOOST_AUTO_TEST_CASE(binary_expression)
{
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Comma), "((func_user_a).v),((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Or), "((func_user_a).v)||((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::And), "((func_user_a).v)&&((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::BitOr), "((func_user_a).v)|((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::BitXor), "((func_user_a).v)^((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::BitAnd), "((func_user_a).v)&((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::SHL), "((func_user_a).v)<<((self->user_b).v)"
    );
    // TODO(scottwe): Token::SAR test
    // TODO(scottwe): Token::SHR test
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Add), "((func_user_a).v)+((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Sub), "((func_user_a).v)-((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Mul), "((func_user_a).v)*((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Div), "((func_user_a).v)/((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Mod), "((func_user_a).v)%((self->user_b).v)"
    );
    // TODO(scottwe): Token::EXP test
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::Equal), "((func_user_a).v)==((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::NotEqual), "((func_user_a).v)!=((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::LessThan), "((func_user_a).v)<((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::GreaterThan), "((func_user_a).v)>((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(_convert_binop(
        Token::LessThanOrEqual), "((func_user_a).v)<=((self->user_b).v)"
    );
    BOOST_CHECK_EQUAL(
        _convert_binop(Token::GreaterThanOrEqual),
        "((func_user_a).v)>=((self->user_b).v)"
    );
}

// Ensures that identifiers are resolved, using the current scope.
BOOST_AUTO_TEST_CASE(identifier_expression)
{
    auto name_a = make_shared<string>("a");
    Identifier id_a(SourceLocation(), name_a);
    Identifier id_b(SourceLocation(), make_shared<string>("b"));

    id_a.annotation().type = new IntegerType(32);
    id_b.annotation().type = new IntegerType(32);

    auto resolver = _prime_resolver(name_a);

    ostringstream a_oss, b_oss, msg_oss;
    a_oss << *ExpressionConverter(id_a, {}, {}, resolver).convert();
    b_oss << *ExpressionConverter(id_b, {}, {}, resolver).convert();

    BOOST_CHECK_EQUAL(a_oss.str(), "(func_user_a).v");
    BOOST_CHECK_EQUAL(b_oss.str(), "(self->user_b).v");
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
