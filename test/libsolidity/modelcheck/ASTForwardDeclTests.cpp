/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/ASTForwardDeclVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/ASTForwardDeclVisitor.h>

#include <test/libsolidity/AnalysisFramework.h>
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

BOOST_FIXTURE_TEST_SUITE(ModelCheckerForwardDecl, ::dev::solidity::test::AnalysisFramework);

BOOST_AUTO_TEST_CASE(simple_contract)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
		}
	)";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_map)
{
    char const* text = R"(
        contract A {
            mapping (uint => uint) a;
        }
    )";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_struct)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            struct B {
                uint a;
                uint b;
            }
        }
    )";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "struct A_B;" << endl;
    oss_expect << "F " << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_modifier)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            modifier simpleModifier {
                require(
                    a >= 100,
                    "Placeholder"
                );
                _;
            }
        }
    )";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "M simpleModifier" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_func)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            function simpleFunc(uint _in) public returns (uint _out) {
                _out = _in;
            }
        }
    )";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "F simpleFunc" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(struct_nesting)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
            struct B {
                mapping (uint => mapping (uint => uint)) a;
            }
		}
	)";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "struct A_B;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "struct A_B_a_submap1;" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(multiple_contracts)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
            struct B {
                mapping (uint => uint) a;
            }
		}
        contract C {
            uint a;
            mapping (uint => uint) b;
        }
	)";
	SourceUnit const* ast = parseAndAnalyse(text);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*ast);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "struct A_B;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "struct C;" << endl;
    oss_expect << "F " << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(nested_maps)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => mapping (uint => uint))) a;
		}
	)";

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*parseAndAnalyse(text));
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;
    oss_expect << "struct A_a_submap1;" << endl;
    oss_expect << "struct A_a_submap2;" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(custom_ctor)
{
    char const* text = R"(
		contract A {
            uint a;
			constructor(uint _a) public {
                a = _a;
            }
		}
	)";
	SourceUnit const* ast = parseAndAnalyse(text);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*ast);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "struct A;" << endl;
    oss_expect << "F " << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
