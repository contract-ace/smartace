/**
 * @date 2019
 * Test suite targeting AST manipulation utilities.
 */

#include <libsolidity/modelcheck/utils/AST.h>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE(ASTUtils)

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

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
