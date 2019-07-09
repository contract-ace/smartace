/*
 * @date 2019
 * A partial test suite to validate key features in the type translation unit.
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>
#include <libsolidity/analysis/GlobalContext.h>
#include <sstream>

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

BOOST_FIXTURE_TEST_SUITE(TypeTranslations, ::dev::solidity::test::AnalysisFramework)

BOOST_AUTO_TEST_CASE(contract_and_structs)
{
    char const* text = R"(
        contract A {
            struct A { int a; }
            struct B { int b; }
            struct C { int c; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(ctrt).name, ctrt.name());
    for (auto structure : ctrt.definedStructs())
    {
        string expt = ctrt.name() + "_" + structure->name();
        BOOST_CHECK_EQUAL(converter.translate(*structure).name, expt);
    }
}

BOOST_AUTO_TEST_CASE(contract_state_variable)
{
    char const* text = "contract A { int a; int b; int c; }";
    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    TypeConverter converter;
    converter.record(ast);

    for (auto decl : ctrt.stateVariables())
    {
        BOOST_CHECK_EQUAL(converter.translate(*decl).name, "int");
    }
}

BOOST_AUTO_TEST_CASE(struct_member_variable)
{
    char const* text = R"(
        contract A {
            struct A { int a; int b; int c; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& strt = *ctrt.definedStructs()[0];

    TypeConverter converter;
    converter.record(ast);

    for (auto decl : strt.members())
    {
        BOOST_CHECK_EQUAL(converter.translate(*decl).name, "int");
    }
}

BOOST_AUTO_TEST_CASE(map_variable)
{
    char const* text = R"(
        contract A {
            mapping (uint => mapping (uint => uint)) arr;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    const auto& map_var = *ctrt.stateVariables()[0];
    const auto& submap1 = *dynamic_cast<Mapping const*>(map_var.typeName());
    const auto& submap2 = *dynamic_cast<Mapping const*>(&submap1.valueType());

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(map_var).name, "A_arr_submap1");
    BOOST_CHECK_EQUAL(converter.translate(submap1).name, "A_arr_submap1");
    BOOST_CHECK_EQUAL(converter.translate(submap2).name, "A_arr_submap2");
}

BOOST_AUTO_TEST_CASE(function)
{
    char const* text = R"(
        contract A {
            function f() public returns (int) { }
            function g() public { }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& funs = ctrt.definedFunctions();

    TypeConverter converter;
    converter.record(ast);

    auto const& f1 = (funs[0]->name() == "f") ? *funs[0] : *funs[1];
    BOOST_CHECK_EQUAL(converter.translate(f1).name, "Method_A_f");
    BOOST_CHECK_EQUAL(converter.translate(f1).type, "int");

    auto const& f2 = (funs[0]->name() != "f") ? *funs[0] : *funs[1];
    BOOST_CHECK_EQUAL(converter.translate(f2).name, "Method_A_g");
    BOOST_CHECK_EQUAL(converter.translate(f2).type, "void");
}

BOOST_AUTO_TEST_CASE(modifier)
{
    char const* text = R"(
        contract A {
            modifier mod1 {
                require(5 == 5);
                _;
            }
            modifier mod2(uint _a) {
                require(_a == 5);
                _;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& mods = ctrt.functionModifiers();

    TypeConverter converter;
    converter.record(ast);

    auto const& m1 = (mods[0]->name() == "mod1") ? *mods[0] : *mods[1];
    BOOST_CHECK_EQUAL(converter.translate(m1).name, "Modifier_A_mod1");
    BOOST_CHECK_EQUAL(converter.translate(m1).type, "void");

    auto const& m2 = (mods[0]->name() != "mod1") ? *mods[0] : *mods[1];
    BOOST_CHECK_EQUAL(converter.translate(m2).name, "Modifier_A_mod2");
    BOOST_CHECK_EQUAL(converter.translate(m2).type, "void");
}

// Integration test which should fail if a new member is added to the global
// context, and that variable is not added to the type translator's lookup. This
// lookup is m_global_context. Success occurs when no exceptions are thrown.
//
// This test works by building a source unit with a single contract, consisting
// of a single function, consisting of expression statements using each global
// context identifier. The resulting contract may not be "valid" due to the use
// of MagicTypes.
BOOST_AUTO_TEST_CASE(global_context_ids)
{
    vector<ASTPointer<Statement>> stmts;
    vector<ASTPointer<VariableDeclaration>> params;
    vector<ASTPointer<ModifierInvocation>> invocs;
    vector<ASTPointer<InheritanceSpecifier>> parents;

    GlobalContext ctx;
    auto decls = ctx.declarations();
    for (auto decl : decls)
    {
        auto name = make_shared<string>(decl->name());
        auto stmt = make_shared<ExpressionStatement>(
            SourceLocation(),
            nullptr,
            make_shared<Identifier>(SourceLocation(), name));

        stmts.push_back(stmt);
    }
    BOOST_CHECK_EQUAL(decls.size(), stmts.size());

    auto func = make_shared<FunctionDefinition>(
        SourceLocation(),
        make_shared<string>("f"),
        Declaration::Visibility::Default,
        StateMutability::NonPayable,
        false,
        nullptr,
        make_shared<ParameterList>(SourceLocation(), params),
        invocs,
        make_shared<ParameterList>(SourceLocation(), params),
        make_shared<Block>(SourceLocation(), nullptr, stmts));
    BOOST_CHECK_EQUAL(func->body().statements().size(), stmts.size());

    auto contract = make_shared<ContractDefinition>(
        SourceLocation(),
        make_shared<string>("A"),
        nullptr,
        parents,
        vector<ASTPointer<ASTNode>>{func});
    BOOST_CHECK_EQUAL(contract->definedFunctions().size(), 1);

    SourceUnit unit(SourceLocation(), {contract});
    BOOST_CHECK_EQUAL(unit.nodes().size(), 1);

    TypeConverter converter;
    converter.record(unit);
}

BOOST_AUTO_TEST_CASE(this_keyword_ids)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IdenExprPtr = Identifier const*;

    char const* text = R"(
        contract A {
            function f() public {
                this;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt = *func.body().statements()[0];
    auto const& expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const& iden = *dynamic_cast<IdenExprPtr>(&expr);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(iden).name, "A");
}

BOOST_AUTO_TEST_CASE(regular_id)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IdenExprPtr = Identifier const*;

    char const* text = R"(
        contract A {
            uint id;
            function f() public {
                id;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt = *func.body().statements()[0];
    auto const& expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const& iden = *dynamic_cast<IdenExprPtr>(&expr);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(iden).name, "unsigned int");
}

BOOST_AUTO_TEST_CASE(contract_access)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using MmbrExprPtr = MemberAccess const*;

    char const* text = R"(
        contract B { uint public i; }
        contract A {
            B b;
            function f() public {
                b.i;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt = *func.body().statements()[0];
    auto const& expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const& mmbr = *dynamic_cast<MmbrExprPtr>(&expr);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(mmbr).name, "unsigned int");
}

BOOST_AUTO_TEST_CASE(struct_access)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using MmbrExprPtr = MemberAccess const*;

    char const* text = R"(
        contract A {
            struct B { mapping (int => int) arr; }
            B b;
            function f() public {
                b.arr;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt = *func.body().statements()[0];
    auto const& expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const& mmbr = *dynamic_cast<MmbrExprPtr>(&expr);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(mmbr).name, "A_B_arr_submap1");
}

BOOST_AUTO_TEST_CASE(map_access)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IndxExprPtr = IndexAccess const*;

    char const* text = R"(
        contract A {
            mapping (int => mapping (int => int)) arr;
            function f() public {
                arr[1][1];
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt = *func.body().statements()[0];
    auto const& expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const& idx1 = *dynamic_cast<IndxExprPtr>(&expr);
    auto const& idx2 = *dynamic_cast<IndxExprPtr>(&idx1.baseExpression());

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(idx1).name, "A_arr_submap2");
    BOOST_CHECK_EQUAL(converter.translate(idx1).type, "int");
    BOOST_CHECK_EQUAL(converter.translate(idx2).name, "A_arr_submap1");
    BOOST_CHECK_EQUAL(converter.translate(idx2).type, "struct A_arr_submap2");
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
