/**
 * @date 2019
 * Targets libsolidity/modelcheck/TypeTranslator.{h,cpp}.
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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

BOOST_FIXTURE_TEST_SUITE(
    TypeTranslations,
    ::dev::solidity::test::AnalysisFramework
)

// Ensures that special structures are annotated with the proper names. Two
// contracts, one containing structures, are defined. They are annotated, and
// then queried to ensure the naming is as expected
BOOST_AUTO_TEST_CASE(contract_and_structs)
{
    char const* text = R"(
        contract A {
            struct A { int a; }
            struct B { int b; }
            struct C { int c; }
        }
        contract B { }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt_a = *retrieveContractByName(ast, "A");
    auto const& ctrt_b = *retrieveContractByName(ast, "B");

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(ctrt_a).name, "A");
    BOOST_CHECK_EQUAL(converter.translate(ctrt_a).type, "struct A");
    BOOST_CHECK_EQUAL(converter.translate(ctrt_b).name, "B");
    BOOST_CHECK_EQUAL(converter.translate(ctrt_b).type, "struct B");
    for (auto structure : ctrt_a.definedStructs())
    {
        string expt_name = ctrt_a.name() + "_" + structure->name();
        string expt_type = "struct " + expt_name;
        BOOST_CHECK_EQUAL(converter.translate(*structure).name, expt_name);
        BOOST_CHECK_EQUAL(converter.translate(*structure).type, expt_type);
    }
}

// Creates a single contract with several state variables. The name and kind of
// each state variable is checked. The state variables are integral to ensure
// that integer annotations are handled.
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
        BOOST_CHECK_EQUAL(converter.translate(*decl).type, "int");
    }
}

// Creates a single contract/struct pair with several state variables. The name
// and kind of each state variable is checked. The state variables are floating
// point types to ensure that integer annotations are handled.
BOOST_AUTO_TEST_CASE(struct_member_variable)
{
    char const* text = R"(
        contract A {
            struct B { fixed128x8 a; fixed128x2 b; fixed128x8 c; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& strt = *ctrt.definedStructs()[0];

    TypeConverter converter;
    converter.record(ast);

    for (auto decl : strt.members())
    {
        BOOST_CHECK_EQUAL(converter.translate(*decl).name, "double");
        BOOST_CHECK_EQUAL(converter.translate(*decl).type, "double");
    }
}

// Tests that map variables are annotated correctly, including in the recursive
// case.
BOOST_AUTO_TEST_CASE(map_variable)
{
    char const* text = R"(
        contract A {
            mapping(uint => mapping(uint => uint)) arr;
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

// Tests that function names and return values are annotated properly. This
// includes functions of void return types.
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

// Tests that constructors are annotated properly.
BOOST_AUTO_TEST_CASE(constructor)
{
    char const* text = R"(
        contract A {
            int a;
            constructor() public { a = 5; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(func).name, "Ctor_A");
    BOOST_CHECK_EQUAL(converter.translate(func).type, "void");
}

// Tests that modifiers are annotated properly.
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
            make_shared<Identifier>(SourceLocation(), name)
        );

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
        make_shared<Block>(SourceLocation(), nullptr, stmts)
    );
    BOOST_CHECK_EQUAL(func->body().statements().size(), stmts.size());

    auto contract = make_shared<ContractDefinition>(
        SourceLocation(),
        make_shared<string>("A"),
        nullptr,
        parents,
        vector<ASTPointer<ASTNode>>{func}
    );
    BOOST_CHECK_EQUAL(contract->definedFunctions().size(), 1);

    SourceUnit unit(SourceLocation(), {contract});
    BOOST_CHECK_EQUAL(unit.nodes().size(), 1);

    TypeConverter converter;
    converter.record(unit);
}

// Tests that the "this" special keyword is mapped back to the present contract.
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

// Tests that identifiers are mapped back to their referenced declarations.
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

// Tests that when resolving identifiers, contract member access is taken into
// account.
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

// Tests that when resolving identifiers, struct member access is taken into
// account.
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

// Tests that map index access calls are treated as function calls, with their
// name being the base to the function call name, and their kind being the
// return value type of said method.
BOOST_AUTO_TEST_CASE(map_access)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IndxExprPtr = IndexAccess const*;

    char const* text = R"(
        contract A {
            mapping(int => mapping(int => int)) arr;
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

// Ensures that storage variable identifiers are mapped to pointers, while
// memory variable identifiers are mapped to new values on the stack.
BOOST_AUTO_TEST_CASE(identifiers_as_pointers)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using IndnExprPtr = Identifier const*;

    char const* text = R"(
        contract A {
            struct B { int i; }
            B b;
            function f() public {
                B storage my_ref = b;
                B memory my_val = B(1);
                my_ref;
                my_val;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt_3 = *func.body().statements()[2];
    auto const& expr_3 = dynamic_cast<ExprStmtPtr>(&stmt_3)->expression();
    auto const& idx1 = *dynamic_cast<IndnExprPtr>(&expr_3);

    auto const& stmt_4 = *func.body().statements()[3];
    auto const& expr_4 = dynamic_cast<ExprStmtPtr>(&stmt_4)->expression();
    auto const& idx2 = *dynamic_cast<IndnExprPtr>(&expr_4);

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.is_pointer(idx1), true);
    BOOST_CHECK_EQUAL(converter.is_pointer(idx2), false);
}

// In Solidity, a function identifier may be encountered before said function is
// declared. This regression test ensures that the TypeConverter handles this
// case by resolving all (relevant) functions before resolving identifiers.
BOOST_AUTO_TEST_CASE(function_and_identifier_oreder_regression)
{
    using ExprStmtPtr = ExpressionStatement const*;
    using FuncExprPtr = FunctionCall const*;
    using MmbrExprPtr = MemberAccess const*;
    using IndnExprPtr = Identifier const*;

    char const* text = R"(
        contract A {
            function f() public { g(); }
            function g() public { }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& func = *ctrt.definedFunctions()[0];

    auto const& stmt = *func.body().statements()[0];
    auto const& expr = dynamic_cast<ExprStmtPtr>(&stmt)->expression();
    auto const& call = *dynamic_cast<FuncExprPtr>(&expr);
    auto const& indx = *dynamic_cast<IndnExprPtr>(&call.expression());

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.translate(indx).name, "Method_A_g");
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
