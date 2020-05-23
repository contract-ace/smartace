/**
 * @date 2019
 * Targets libsolidity/modelcheck/TypeTranslator.{h,cpp}.
 */

#include <libsolidity/modelcheck/analysis/Types.h>

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

    BOOST_CHECK(converter.has_record(ctrt_a));
    BOOST_CHECK_EQUAL(converter.get_name(ctrt_a), "A");
    BOOST_CHECK_EQUAL(converter.get_type(ctrt_a), "struct A");
    BOOST_CHECK(converter.has_record(ctrt_b));
    BOOST_CHECK_EQUAL(converter.get_name(ctrt_b), "B");
    BOOST_CHECK_EQUAL(converter.get_type(ctrt_b), "struct B");
    for (auto structure : ctrt_a.definedStructs())
    {
        string expt_name = ctrt_a.name() + "_Struct_" + structure->name();
        string expt_type = "struct " + expt_name;
        BOOST_CHECK(converter.has_record(*structure));
        BOOST_CHECK_EQUAL(converter.get_name(*structure), expt_name);
        BOOST_CHECK_EQUAL(converter.get_type(*structure), expt_type);
    }
}

// Attemps to create a contract with all "simple types" covered as member
// variables. That is bools, addresses, and both the signed and unsigned
// variants of all numeric types, varied along each parameter (bits, etc).
BOOST_AUTO_TEST_CASE(simple_types)
{
    char const* text = R"(
        contract A {
            enum MyEnum { A, B, C }
            bool a;
            address b;
            int32 c;
            int40 d;
            uint32 e;
            uint40 f;
            fixed32x10 g;
            fixed40x11 h;
            ufixed32x10 i;
            ufixed40x11 j;
            MyEnum k;
        }
    )";

    map<string, string> const EXPECTED = {
        {"a", "sol_bool_t"}, {"b", "sol_address_t"}, {"c", "sol_int32_t"},
        {"d", "sol_int40_t"}, {"e", "sol_uint32_t"}, {"f", "sol_uint40_t"},
        {"g", "sol_fixed32X10_t"}, {"h", "sol_fixed40X11_t"},
        {"i", "sol_ufixed32X10_t"}, {"j", "sol_ufixed40X11_t"},
        {"k", "sol_uint8_t"}
    };

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    TypeConverter converter;
    converter.record(ast);

    for (auto member : ctrt.stateVariables())
    {
        string const& EXPT = EXPECTED.find(member->name())->second;
        BOOST_CHECK_EQUAL(converter.get_type(*member), EXPT);
    }
}

// Creates a single contract with several state variables. The name and kind of
// each state variable is checked.
BOOST_AUTO_TEST_CASE(contract_state_variable)
{
    char const* text = "contract A { int a; int b; int c; }";
    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");

    TypeConverter converter;
    converter.record(ast);

    for (auto decl : ctrt.stateVariables())
    {
        BOOST_CHECK(converter.has_record(*decl));
        BOOST_CHECK_EQUAL(converter.get_type(*decl), "sol_int256_t");
        BOOST_CHECK_THROW(converter.get_name(*decl), runtime_error);
    }
}

// Creates a single contract/struct pair with several state variables. The name
// and kind of each state variable is checked.
BOOST_AUTO_TEST_CASE(struct_member_variable)
{
    char const* text = R"(
        contract A {
            struct B { fixed128x8 a; fixed128x8 b; fixed128x8 c; }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A");
    auto const& strt = *ctrt.definedStructs()[0];

    TypeConverter converter;
    converter.record(ast);

    for (auto decl : strt.members())
    {
        BOOST_CHECK(converter.has_record(*decl));
        BOOST_CHECK_EQUAL(converter.get_type(*decl), "sol_fixed128X8_t");
        BOOST_CHECK_THROW(converter.get_name(*decl), runtime_error);
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

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.get_name(map_var), "Map_1");
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
    BOOST_CHECK(converter.has_record(f1));
    BOOST_CHECK_EQUAL(converter.get_type(f1), "sol_int256_t");

    auto const& f2 = (funs[0]->name() != "f") ? *funs[0] : *funs[1];
    BOOST_CHECK(converter.has_record(f2));
    BOOST_CHECK_EQUAL(converter.get_type(f2), "void");
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
    func->setScope(contract.get());
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

    BOOST_CHECK_EQUAL(converter.get_name(iden), "A");
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

    BOOST_CHECK_EQUAL(converter.get_type(iden), "sol_uint256_t");
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

    BOOST_CHECK_EQUAL(converter.get_type(mmbr), "sol_uint256_t");
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

    BOOST_CHECK_EQUAL(converter.get_name(mmbr), "Map_1");
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

// Ensures names are escaped, as per the translation specifications.
BOOST_AUTO_TEST_CASE(name_escape)
{
    char const* text = R"(
        contract A_B {
            struct C_D { int i; }
            function f_func() public { }
            mapping(int => int) m_map;
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto const& ctrt = *retrieveContractByName(ast, "A_B");
    auto const& strt = *ctrt.definedStructs()[0];
    auto const& mapv = *ctrt.stateVariables()[0];

    TypeConverter converter;
    converter.record(ast);

    BOOST_CHECK_EQUAL(converter.get_name(ctrt), "A__B");
    BOOST_CHECK_EQUAL(converter.get_name(strt), "A__B_Struct_C__D");
    BOOST_CHECK_EQUAL(converter.get_name(mapv), "Map_1");
}

BOOST_AUTO_TEST_CASE(bounded_addr)
{
    TypeConverter converter;
    converter.limit_addresses(10);

    AddressType type(StateMutability::Payable);
    auto raw_nd = converter.raw_simple_nd(type, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "nd_range(0,10,\"Blah\")");
}

BOOST_AUTO_TEST_CASE(bounded_bool)
{
    TypeConverter converter;

    BoolType type;
    auto raw_nd = converter.raw_simple_nd(type, "Blah");

    std::ostringstream expr;
    expr << *raw_nd;
    BOOST_CHECK_EQUAL(expr.str(), "nd_range(0,2,\"Blah\")");
}

BOOST_AUTO_TEST_SUITE_END()

}
}
}
}
