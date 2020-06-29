/**
 * Tests for libsolidity/modelcheck/analysis/ContractRvAnalysis.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>

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

BOOST_FIXTURE_TEST_SUITE(
    Analysis_ContractRvAnalysisTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(basic_rv_analysis)
{
    char const* text = R"(
        contract X {}
        contract XX is X {}
        contract Test {
            X x;
            XX xx;
            constructor() public {
                x = new X();
                xx = new XX();
            }
            function f(bool flag) public view returns (X) {
                if (flag) {
                    return x;
                }
                else {
                    return xx;
                }
            }
            function g() public view returns (X) {
                return XX(address(5));
            }
            function h(bool flag) public view returns (X) {
                if (flag) {
                    return f(false);
                }
                else if (flag) {
                    return X(address(5));
                }
                else {
                    return xx;
                }
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "Test");

    auto child_x = retrieveContractByName(ast, "X");
    auto child_xx = retrieveContractByName(ast, "XX");

    auto func_f = ctrt->definedFunctions()[1];
    auto func_g = ctrt->definedFunctions()[2];
    auto func_h = ctrt->definedFunctions()[3];

    BOOST_CHECK_EQUAL(func_f->name(), "f");
    BOOST_CHECK_EQUAL(func_g->name(), "g");
    BOOST_CHECK_EQUAL(func_h->name(), "h");

    vector<ContractDefinition const*> model({ ctrt });
    auto graph = make_shared<AllocationGraph>(model);

    ContractRvAnalyzer f_res(*ctrt, graph, *func_f);
    BOOST_CHECK_EQUAL(f_res.internals().size(), 2);
    BOOST_CHECK(f_res.internals().find(child_x) != f_res.internals().end());
    BOOST_CHECK(f_res.internals().find(child_xx) != f_res.internals().end());
    BOOST_CHECK_EQUAL(f_res.externals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.dependencies().size(), 0);

    ContractRvAnalyzer g_res(*ctrt, graph, *func_g);
    BOOST_CHECK_EQUAL(g_res.internals().size(), 0);
    BOOST_CHECK_EQUAL(g_res.externals().size(), 1);
    BOOST_CHECK(g_res.externals().find(child_xx) != g_res.externals().end());
    BOOST_CHECK_EQUAL(g_res.dependencies().size(), 0);

    ContractRvAnalyzer h_res(*ctrt, graph, *func_h);
    BOOST_CHECK_EQUAL(h_res.internals().size(), 1);
    BOOST_CHECK(h_res.externals().find(child_xx) != h_res.externals().end());
    BOOST_CHECK_EQUAL(h_res.externals().size(), 1);
    BOOST_CHECK(h_res.externals().find(child_x) != h_res.externals().end());
    BOOST_CHECK_EQUAL(h_res.dependencies().size(), 1);
    BOOST_CHECK(h_res.dependencies().find(func_f) != h_res.dependencies().end());
}

BOOST_AUTO_TEST_CASE(library_call)
{
    char const* text = R"(
        contract X {}
        library Lib {
            function f() internal returns (X) {
                return new X();
            }
        }
        contract Test {
            function f() public returns (X) {
                return Lib.f();
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "Test");

    auto lib = retrieveContractByName(ast, "Lib");

    auto func_f = ctrt->definedFunctions()[0];
    auto libcall = lib->definedFunctions()[0];

    BOOST_CHECK_EQUAL(func_f->name(), "f");
    BOOST_CHECK_EQUAL(libcall->name(), "f");

    vector<ContractDefinition const*> model({ ctrt });
    auto graph = make_shared<AllocationGraph>(model);

    ContractRvAnalyzer f_res(*ctrt, graph, *func_f);
    BOOST_CHECK_EQUAL(f_res.internals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.externals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.dependencies().size(), 1);
    BOOST_CHECK(f_res.dependencies().find(libcall) != f_res.dependencies().end());
}

BOOST_AUTO_TEST_CASE(supercall)
{
    char const* text = R"(
        contract X {}
        contract Base {
            function f() public returns (X) {
                return new X();
            }
        }
        contract Test is Base {
            function f() public returns (X) {
                return super.f();
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "Test");

    auto base = retrieveContractByName(ast, "Base");

    auto func_f = ctrt->definedFunctions()[0];
    auto super = base->definedFunctions()[0];

    BOOST_CHECK_EQUAL(func_f->name(), "f");
    BOOST_CHECK_EQUAL(super->name(), "f");

    vector<ContractDefinition const*> model({ ctrt });
    auto graph = make_shared<AllocationGraph>(model);

    ContractRvAnalyzer f_res(*ctrt, graph, *func_f);
    BOOST_CHECK_EQUAL(f_res.internals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.externals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.dependencies().size(), 1);
    BOOST_CHECK(f_res.dependencies().find(super) != f_res.dependencies().end());
}

BOOST_AUTO_TEST_CASE(external_call)
{
    char const* text = R"(
        contract X {}
        contract Base {
            X x;
            constructor() public { x = new X(); }
            function f() public view returns (X) { return x; }
        }
        contract Ext is Base {
            function f() public view returns (X) { return x; }
        }
        contract Test {
            Ext ext;
            constructor() public { ext = new Ext(); }
            function f() public view returns (X) {
                return ext.f();
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "Test");

    auto ext = retrieveContractByName(ast, "Ext");

    auto func_f = ctrt->definedFunctions()[1];
    auto extcall = ext->definedFunctions()[1];

    BOOST_CHECK_EQUAL(func_f->name(), "f");
    BOOST_CHECK_EQUAL(extcall->name(), "f");

    vector<ContractDefinition const*> model({ ctrt });
    auto graph = make_shared<AllocationGraph>(model);

    ContractRvAnalyzer f_res(*ctrt, graph, *func_f);
    BOOST_CHECK_EQUAL(f_res.internals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.externals().size(), 0);
    BOOST_CHECK_EQUAL(f_res.dependencies().size(), 1);
    BOOST_CHECK(f_res.dependencies().find(extcall) != f_res.dependencies().end());
}

BOOST_AUTO_TEST_CASE(unsupported)
{
    char const* text = R"(
        contract X {}
        contract Aux {
            X x;
            constructor() public { x = new X(); }
            function f() public view returns (X) { return x; }
        }
        contract Test {
            Aux aux;
            constructor() public { aux = new Aux(); }
            function get_aux() public view returns (Aux) { return aux; }
            function f() public view returns (X) {
                return get_aux().f();
            }
            function g() public view returns (X) {
                X x_copy;
                return x_copy;
            }
        }
    )";

    auto const& ast = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(ast, "Test");

    auto func_f = ctrt->definedFunctions()[2];
    auto func_g = ctrt->definedFunctions()[3];

    BOOST_CHECK_EQUAL(func_f->name(), "f");
    BOOST_CHECK_EQUAL(func_g->name(), "g");

    vector<ContractDefinition const*> model({ ctrt });
    auto graph = make_shared<AllocationGraph>(model);

    BOOST_CHECK_THROW(ContractRvAnalyzer(*ctrt, graph, *func_f), runtime_error);
    BOOST_CHECK_THROW(ContractRvAnalyzer(*ctrt, graph, *func_g), runtime_error);
}

BOOST_AUTO_TEST_CASE(resolve_id)
{
    char const* text = R"(
        contract X {}
        contract Y {}
        contract Test {
            X x;
            Y y;
            constructor() public {
                x = new X();
                y = new Y();
            }
            function f() public view { x; }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "Test");
    
    auto stmt = ctrt->definedFunctions()[0]->body().statements()[0];
    auto expr_stmt = dynamic_cast<ExpressionStatement const*>(stmt.get());
    auto assignment = dynamic_cast<Assignment const*>(&expr_stmt->expression());
    auto id = (&assignment->leftHandSide());

    vector<ContractDefinition const*> model({ ctrt });
    auto graph = make_shared<AllocationGraph>(model);

    FlatModel flat_model(model, *graph);

    ContractExpressionAnalyzer resolver(flat_model, graph);
    BOOST_CHECK_EQUAL(resolver.resolve(*id).name(), "X");
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
