/**
 * @date 2019
 * Comprehensive tests for libsolidity/modelcheck/ASTForwardDeclVisitor.{h,cpp}.
 */

#include <libsolidity/modelcheck/ADTForwardDeclVisitor.h>
#include <libsolidity/modelcheck/FunctionForwardDeclVisitor.h>

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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Ctor_A" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_map)
{
    char const* text = R"(
        contract A {
            mapping (uint => uint) a;
        }
    )";

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_a_submap1;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "unsigned int "
                << "Read_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "struct A_B Ctor_A_B" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "M simpleModifier" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "unsigned int Method_A_simpleFunc" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_void_func)
{
    char const* text = R"(
        contract A {
			uint a;
            uint b;
            function simpleFunc(uint _in) public {
                a = _in;
            }
        }
    )";

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "void Method_A_simpleFunc" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    adt_expect << "struct A_B_a_submap2;" << endl;
    adt_expect << "struct A_B_a_submap1;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "struct A_B Ctor_A_B" << endl;
    func_expect << "struct A_B_a_submap1 "
                << "Read_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx, struct A_B_a_submap1 d);"
                << endl;
    func_expect << "struct A_B_a_submap1 *"
                << "Ref_A_B_a_submap2"
                << "(struct A_B_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "unsigned int "
                << "Read_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    adt_expect << "struct A_B_a_submap1;" << endl;
    adt_expect << "struct C;" << endl;
    adt_expect << "struct C_b_submap1;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "struct A_B Ctor_A_B" << endl;
    func_expect << "unsigned int "
                << "Read_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_B_a_submap1"
                << "(struct A_B_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "struct C Ctor_C" << endl;
    func_expect << "unsigned int "
                << "Read_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_C_b_submap1"
                << "(struct C_b_submap1 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(nested_maps)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => mapping (uint => uint))) a;
		}
	)";

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_a_submap3;" << endl;
    adt_expect << "struct A_a_submap2;" << endl;
    adt_expect << "struct A_a_submap1;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "struct A_a_submap2 "
                << "Read_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx, struct A_a_submap2 d);"
                << endl;
    func_expect << "struct A_a_submap2 *"
                << "Ref_A_a_submap3"
                << "(struct A_a_submap3 *a, unsigned int idx);"
                << endl;
    func_expect << "struct A_a_submap1 "
                << "Read_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx, struct A_a_submap1 d);"
                << endl;
    func_expect << "struct A_a_submap1 *"
                << "Ref_A_a_submap2"
                << "(struct A_a_submap2 *a, unsigned int idx);"
                << endl;
    func_expect << "unsigned int "
                << "Read_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;
    func_expect << "void "
                << "Write_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx, unsigned int d);"
                << endl;
    func_expect << "unsigned int *"
                << "Ref_A_a_submap1"
                << "(struct A_a_submap1 *a, unsigned int idx);"
                << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
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

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    func_expect << "struct A Ctor_A" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_CASE(nontrivial_retval)
{
    char const* text = R"(
        pragma experimental ABIEncoderV2;
        contract A {
			uint a;
            uint b;
            struct B {
                uint a;
            }
            function advFunc(uint _in) public returns (B memory _out) {
                _out = B(_in);
            }
        }
    )";

    const auto &ast = *parseAndAnalyse(text);

    ostringstream adt_actual, func_actual;
    ADTForwardDeclVisitor adt_visitor(ast);
    FunctionForwardDeclVisitor func_visitor(ast);
    adt_visitor.print(adt_actual);
    func_visitor.print(func_actual);

    ostringstream adt_expect, func_expect;
    adt_expect << "struct A;" << endl;
    adt_expect << "struct A_B;" << endl;
    func_expect << "struct A Ctor_A" << endl;
    func_expect << "struct A_B Ctor_A_B" << endl;
    func_expect << "struct A_B Method_A_advFunc" << endl;

    BOOST_CHECK_EQUAL(adt_actual.str(), adt_expect.str());
    BOOST_CHECK_EQUAL(func_actual.str(), func_expect.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
