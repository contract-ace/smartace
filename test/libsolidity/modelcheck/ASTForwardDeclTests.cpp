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

BOOST_AUTO_TEST_CASE(simple_map)
{
    char const* text = R"(
        contract A {
            mapping (uint => uint) a;
        }
    )";
	ContractDefinition const* contract;
    VariableDeclaration const* u2umap;
	SourceUnit const* ast = parseAndAnalyse(text);
    BOOST_REQUIRE((contract = retrieveContractByName(*ast, "A")) != nullptr);
    BOOST_REQUIRE((u2umap = contract->stateVariables()[0]) != nullptr);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*u2umap);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "A" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_struct)
{
    char const* text = R"(
        contract A {
            struct B {
                uint a;
                uint b;
            }
        }
    )";
	ContractDefinition const* contract;
    StructDefinition const* struc;
	SourceUnit const* ast = parseAndAnalyse(text);
    BOOST_REQUIRE((contract = retrieveContractByName(*ast, "A")) != nullptr);
    BOOST_REQUIRE((struc = contract->definedStructs()[0]) != nullptr);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*struc);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "S B" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(simple_contract)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
		}
	)";
	SourceUnit const* ast = parseAndAnalyse(text);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*ast);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "C A" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(contract_nesting)
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
	SourceUnit const* ast = parseAndAnalyse(text);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*ast);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "C A" << endl;
    oss_expect << "S B" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(struct_nesting)
{
    char const* text = R"(
		contract A {
			uint a;
            uint b;
            struct B {
                mapping (uint => uint) a;
            }
		}
	)";
	ContractDefinition const* contract;
    StructDefinition const* struc;
	SourceUnit const* ast = parseAndAnalyse(text);
    BOOST_REQUIRE((contract = retrieveContractByName(*ast, "A")) != nullptr);
    BOOST_REQUIRE((struc = contract->definedStructs()[0]) != nullptr);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*struc);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "S B" << endl;
    oss_expect << "A" << endl;

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
    oss_expect << "C A" << endl;
    oss_expect << "S B" << endl;
    oss_expect << "A" << endl;
    oss_expect << "C C" << endl;
    oss_expect << "A" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_CASE(nested_maps)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => uint)) a;
		}
	)";
	ContractDefinition const* contract;
    VariableDeclaration const* u2umap;
	SourceUnit const* ast = parseAndAnalyse(text);
    BOOST_REQUIRE((contract = retrieveContractByName(*ast, "A")) != nullptr);
    BOOST_REQUIRE((u2umap = contract->stateVariables()[0]) != nullptr);

    ostringstream oss_actual;
    ASTForwardDeclVisitor decl_visitor(*u2umap);
    decl_visitor.print(oss_actual);

    ostringstream oss_expect;
    oss_expect << "A" << endl;
    oss_expect << "A" << endl;

    BOOST_CHECK_EQUAL(oss_actual.str(), oss_expect.str());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
