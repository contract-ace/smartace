/**
 * Tests for libsolidity/modelcheck/analysis/FunctionCall.
 * 
 * @date 2019
 */

#include <libsolidity/modelcheck/analysis/FunctionCall.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

// -------------------------------------------------------------------------- //

namespace
{

class CallSearch: public ASTConstVisitor
{
public:
    CallSearch(ASTPointer<Statement const> _node)
    {
        _node->accept(*this);
    }

    FunctionCall const* call;
    vector<ASTPointer<Expression const>> args;

protected:
    bool visit(FunctionCall const& _node)
    {
        call = &_node;
        args = _node.arguments();
        return false;
    }
};

class LiteralSearch: public ASTConstVisitor
{
public:
    LiteralSearch(ASTPointer<Expression const> _node)
    {
        if (_node != nullptr) _node->accept(*this);
    }

    int64_t v = -1;

protected:
    bool visit(Literal const& _node)
    {
        v = stoi(_node.value());
        return false;
    }
};

}

// -------------------------------------------------------------------------- //

BOOST_FIXTURE_TEST_SUITE(
    Analysis_FunctionCallTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(annotation_tests)
{
    char const* text = R"(
        contract X {
            function h(int x) public payable {}
        }
        contract Test {
            X x;
            function f() public {}
            function g(address _addr) public {
                f();
                x.h(1);
                x.h.gas(1)(2);
                x.h.value(1)(2);
                x.h.gas(1).value(2)(3);
                x.h.value(1).gas(2)(3);
                this.f();
                _addr.call.value(5)("");
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* x = retrieveContractByName(unit, "Test");
    auto const& stmts = x->definedFunctions()[1]->body().statements();
    BOOST_CHECK_EQUAL(stmts.size(), 8);

    CallSearch call1(stmts[0]);
    FunctionCallAnalyzer test1(*call1.call);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        test1.args().begin(), test1.args().end(),
        call1.args.begin(), call1.args.end()
    );
    BOOST_CHECK_EQUAL(test1.value(), nullptr);
    BOOST_CHECK_EQUAL(test1.gas(), nullptr);
    BOOST_CHECK_EQUAL(test1.context(), nullptr);
    BOOST_CHECK(test1.classify() == FunctionCallAnalyzer::CallGroup::Method);
    BOOST_CHECK(!test1.is_in_library());
    BOOST_CHECK(!test1.is_super());
    BOOST_CHECK(!test1.is_low_level());

    CallSearch call2(stmts[1]);
    FunctionCallAnalyzer test2(*call2.call);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        test2.args().begin(), test2.args().end(),
        call2.args.begin(), call2.args.end()
    );
    BOOST_CHECK_EQUAL(test2.value(), nullptr);
    BOOST_CHECK_EQUAL(test2.gas(), nullptr);
    BOOST_CHECK_NE(test2.context(), nullptr);
    BOOST_CHECK(!test2.context_is_this());
    BOOST_CHECK(!test2.is_low_level());

    CallSearch call3(stmts[2]);
    FunctionCallAnalyzer test3(*call3.call);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        test3.args().begin(), test3.args().end(),
        call3.args.begin(), call3.args.end()
    );
    BOOST_CHECK_EQUAL(test3.value(), nullptr);
    BOOST_CHECK_EQUAL(LiteralSearch(test3.gas()).v, 1);
    BOOST_CHECK_NE(test3.context(), nullptr);
    BOOST_CHECK(!test3.context_is_this());
    BOOST_CHECK(!test3.is_low_level());

    CallSearch call4(stmts[3]);
    FunctionCallAnalyzer test4(*call4.call);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        test4.args().begin(), test4.args().end(),
        call4.args.begin(), call4.args.end()
    );
    BOOST_CHECK_EQUAL(LiteralSearch(test4.value()).v, 1);
    BOOST_CHECK_EQUAL(test4.gas(), nullptr);
    BOOST_CHECK_NE(test4.context(), nullptr);
    BOOST_CHECK(!test4.is_low_level());

    CallSearch call5(stmts[4]);
    FunctionCallAnalyzer test5(*call5.call);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        test5.args().begin(), test5.args().end(),
        call5.args.begin(), call5.args.end()
    );
    BOOST_CHECK_EQUAL(LiteralSearch(test5.value()).v, 2);
    BOOST_CHECK_EQUAL(LiteralSearch(test5.gas()).v, 1);
    BOOST_CHECK_NE(test5.context(), nullptr);
    BOOST_CHECK(!test5.is_low_level());

    CallSearch call6(stmts[5]);
    FunctionCallAnalyzer test6(*call6.call);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        test6.args().begin(), test6.args().end(),
        call6.args.begin(), call6.args.end()
    );
    BOOST_CHECK_EQUAL(LiteralSearch(test6.value()).v, 1);
    BOOST_CHECK_EQUAL(LiteralSearch(test6.gas()).v, 2);
    BOOST_CHECK_NE(test6.context(), nullptr);
    BOOST_CHECK(!test6.is_low_level());

    CallSearch call7(stmts[6]);
    FunctionCallAnalyzer test7(*call7.call);
    BOOST_CHECK(test7.context_is_this());
    BOOST_CHECK(!test7.is_low_level());

    CallSearch call8(stmts[7]);
    FunctionCallAnalyzer test8(*call8.call);
    BOOST_CHECK_EQUAL(test8.args().size(), 1);
    BOOST_CHECK_EQUAL(LiteralSearch(test8.value()).v, 5);
    BOOST_CHECK_EQUAL(test8.gas(), nullptr);
    BOOST_CHECK_NE(test8.context(), nullptr);
    BOOST_CHECK(test8.classify() == FunctionCallAnalyzer::CallGroup::Method);
    BOOST_CHECK(!test8.is_in_library());
    BOOST_CHECK(!test8.is_super());
    BOOST_CHECK(test8.is_low_level());
}

BOOST_AUTO_TEST_CASE(special_annotations)
{
    char const* text = R"(
        library Lib {
            function incr(uint256 x) internal {}
            function f() public pure {}
        }
        contract X {
            function g(uint256 _x) public {}
        }
        contract Test is X {
            using Lib for uint256;
            function g(uint256 _x) public {
                _x.incr();
                super.g(_x);
                assert(_x != 5);
                Lib.f();
                X.g(_x);
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);

    auto const* x = retrieveContractByName(unit, "Test");
    auto const& stmts = x->definedFunctions()[0]->body().statements();
    BOOST_CHECK_EQUAL(stmts.size(), 5);

    CallSearch call1(stmts[0]);
    FunctionCallAnalyzer test1(*call1.call);
    BOOST_CHECK(test1.is_in_library());
    BOOST_CHECK(!test1.is_super());

    CallSearch call2(stmts[1]);
    FunctionCallAnalyzer test2(*call2.call);
    BOOST_CHECK(test2.is_super());

    CallSearch call3(stmts[2]);
    FunctionCallAnalyzer test3(*call3.call);
    BOOST_CHECK(test3.classify() == FunctionCallAnalyzer::CallGroup::Assert);
    BOOST_CHECK(!test3.is_super());

    CallSearch call4(stmts[3]);
    FunctionCallAnalyzer test4(*call4.call);
    BOOST_CHECK(test4.is_in_library());
    BOOST_CHECK_EQUAL(test4.context(), nullptr);
    BOOST_CHECK(!test4.is_super());

    CallSearch call5(stmts[4]);
    FunctionCallAnalyzer test5(*call5.call);
    BOOST_CHECK(test5.is_super());
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
