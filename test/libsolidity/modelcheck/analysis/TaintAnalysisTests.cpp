/**
 * Tests for libsolidity/modelcheck/analysis/TaintAnalysis.
 * 
 * @date 2021
 */

#include <libsolidity/modelcheck/analysis/TaintAnalysis.h>

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

BOOST_FIXTURE_TEST_SUITE(
    Analysis_TaintAnalysisTests, ::dev::solidity::test::AnalysisFramework
)

namespace
{
void taint_check(
    TaintAnalysis & _analysis,
    VariableDeclaration const& _decl,
    set<size_t> _expected,
    size_t _sources
)
{
    auto const& taint = _analysis.taint_for(_decl);
    BOOST_CHECK_EQUAL(taint.size(), _sources);
    for (size_t i = 0; i < _sources; ++i)
    {
        if (_expected.find(i) != _expected.end())
        {
            BOOST_CHECK(taint[i]);
        }
        else
        {
            BOOST_CHECK(!taint[i]);
        }
    }
}
}

BOOST_AUTO_TEST_CASE(no_taint)
{
    char const* text = R"(
        contract A {
            function f(int x, int y, int z) public returns (int) {
                int w = x + y;
                int q = w + x;
                return 2 * w + q;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 10;
    auto const& func = *ctrt->definedFunctions()[0];
    TaintAnalysis analysis(sources);
    analysis.run(func);

    for (auto param : func.parameters())
    {
        taint_check(analysis, *param.get(), {}, sources);
    }

    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            taint_check(analysis, *decl, {}, sources);
        }
    }
}

BOOST_AUTO_TEST_CASE(single_taint)
{
    char const* text = R"(
        contract A {
            function f(int x, int y, int z) public returns (int) {
                int w = x + y;
                int q = x + z;
                int ret = 2 * w + q;
                return ret;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 10;
    auto const& func = *ctrt->definedFunctions()[0];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 2;
    size_t tag_2 = 5;
    set<size_t> tags{ tag_1, tag_2 };
    auto const& tainted_source = (*func.parameters()[1].get());
    analysis.taint(tainted_source, tag_1);
    analysis.taint(tainted_source, tag_2);

    taint_check(analysis, tainted_source, tags, sources);

    analysis.run(func);

    for (auto param : func.parameters())
    {
        if (param->name() == "y")
        {
            taint_check(analysis, *param.get(), tags, sources);
        }
        else
        {
            taint_check(analysis, *param.get(), {}, sources);
        }
    }

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "w" || decl->name() == "ret")
            {
                taint_check(analysis, *decl, tags, sources);
            }
            else
            {
                taint_check(analysis, *decl, {}, sources);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 3);
}

BOOST_AUTO_TEST_CASE(multiple_taint)
{
    char const* text = R"(
        contract A {
            function f(int x, int y, int z) public returns (int) {
                int w = x + z;
                int q = y + z;
                int u = z * z;
                int ret = w + q + u;
                return ret;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 10;
    auto const& func = *ctrt->definedFunctions()[0];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 2;
    size_t tag_2 = 5;
    set<size_t> x_tag{ tag_1 };
    set<size_t> y_tag{ tag_2 };
    set<size_t> mixed_tag{ tag_1, tag_2 };
    auto const& input_x = (*func.parameters()[0].get());
    auto const& input_y = (*func.parameters()[1].get());
    auto const& input_z = (*func.parameters()[2].get());

    analysis.taint(input_x, tag_1);
    taint_check(analysis, input_x, x_tag, sources);
    analysis.taint(input_y, tag_2);
    taint_check(analysis, input_y, y_tag, sources);

    analysis.run(func);

    taint_check(analysis, input_x, x_tag, sources);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, {}, sources);

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "w")
            {
                taint_check(analysis, *decl, x_tag, sources);
            }
            else if (decl->name() == "q")
            {
                taint_check(analysis, *decl, y_tag, sources);
            }
            else if (decl->name() == "ret")
            {
                taint_check(analysis, *decl, mixed_tag, sources);
            }
            else
            {
                taint_check(analysis, *decl, {}, sources);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 4);
}

BOOST_AUTO_TEST_CASE(multiple_passes)
{
    char const* text = R"(
        contract A {
            function f(int x, int y, int z) public returns (int) {
                int a = x;
                int b = y;
                int c = z;
                while (a != b)
                {
                    a = a + b;
                    b = b + c;
                }
                int ret = a;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 10;
    auto const& func = *ctrt->definedFunctions()[0];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 2;
    size_t tag_2 = 5;
    size_t tag_3 = 7;
    set<size_t> x_tag{ tag_1 };
    set<size_t> y_tag{ tag_2 };
    set<size_t> z_tag{ tag_3 };
    set<size_t> yz_tag{ tag_2, tag_3 };
    set<size_t> xyz_tag{ tag_1, tag_2, tag_3 };
    auto const& input_x = (*func.parameters()[0].get());
    auto const& input_y = (*func.parameters()[1].get());
    auto const& input_z = (*func.parameters()[2].get());

    analysis.taint(input_x, tag_1);
    taint_check(analysis, input_x, x_tag, sources);
    analysis.taint(input_y, tag_2);
    taint_check(analysis, input_y, y_tag, sources);
    analysis.taint(input_z, tag_3);
    taint_check(analysis, input_z, z_tag, sources);

    analysis.run(func);

    taint_check(analysis, input_x, x_tag, sources);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, z_tag, sources);

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "a")
            {
                taint_check(analysis, *decl, xyz_tag, sources);
            }
            else if (decl->name() == "b")
            {
                taint_check(analysis, *decl, yz_tag, sources);
            }
            else if (decl->name() == "c")
            {
                taint_check(analysis, *decl, z_tag, sources);
            }
            else if (decl->name() == "ret")
            {
                taint_check(analysis, *decl, xyz_tag, sources);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 4);
}

BOOST_AUTO_TEST_CASE(fn_call)
{
    char const* text = R"(
        contract A {
            function g(int x) public returns (int) { return x; }
            function f(int x, int y, int z) public returns (int) {
                int a = x;
                int b = y;
                int c = 0;
                while (a != b)
                {
                    a = a + c;
                    c = g(b);
                }
                int ret = c;
                return c;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 5;
    auto const& func = *ctrt->definedFunctions()[1];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 0;
    size_t tag_2 = 1;
    size_t tag_3 = 2;
    size_t tag_4 = 3;
    size_t tag_5 = 4;
    set<size_t> x_tag{ tag_1 };
    set<size_t> y_tag{ tag_2 };
    set<size_t> z_tag{ tag_3 };
    set<size_t> fn_tag{ tag_1, tag_2, tag_3, tag_4, tag_5 };
    auto const& input_x = (*func.parameters()[0].get());
    auto const& input_y = (*func.parameters()[1].get());
    auto const& input_z = (*func.parameters()[2].get());

    analysis.taint(input_x, tag_1);
    taint_check(analysis, input_x, x_tag, sources);
    analysis.taint(input_y, tag_2);
    taint_check(analysis, input_y, y_tag, sources);
    analysis.taint(input_z, tag_3);
    taint_check(analysis, input_z, z_tag, sources);

    analysis.run(func);

    taint_check(analysis, input_x, x_tag, sources);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, z_tag, sources);

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "a")
            {
                taint_check(analysis, *decl, fn_tag, sources);
            }
            else if (decl->name() == "b")
            {
                taint_check(analysis, *decl, y_tag, sources);
            }
            else if (decl->name() == "c")
            {
                taint_check(analysis, *decl, fn_tag, sources);
            }
            else if (decl->name() == "ret")
            {
                taint_check(analysis, *decl, fn_tag, sources);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 4);
}

BOOST_AUTO_TEST_CASE(structs)
{
    char const* text = R"(
        contract A {
            struct S { int a; int b; }
            function f(int x, int y, int z, int w) public returns (int) {
                S memory s1;
                S memory s2;
                s1.a = x;
                s2.a = y;
                int a;
                int b;
                a = s1.a;
                b = s2.a;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 30;
    auto const& func = *ctrt->definedFunctions()[0];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 4;
    size_t tag_2 = 9;
    set<size_t> x_tag{ tag_1 };
    set<size_t> y_tag{ tag_2 };
    auto const& input_x = (*func.parameters()[0].get());
    auto const& input_y = (*func.parameters()[1].get());
    auto const& input_z = (*func.parameters()[2].get());
    auto const& input_w = (*func.parameters()[3].get());

    analysis.taint(input_x, tag_1);
    taint_check(analysis, input_x, x_tag, sources);
    analysis.taint(input_y, tag_2);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, {}, sources);
    taint_check(analysis, input_w, {}, sources);

    analysis.run(func);

    taint_check(analysis, input_x, x_tag, sources);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, {}, sources);
    taint_check(analysis, input_w, {}, sources);

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "a")
            {
                    auto const& taint = analysis.taint_for(*decl);
                    BOOST_CHECK_EQUAL(taint.size(), sources);
                    BOOST_CHECK(taint[tag_1]);
            }
            else if (decl->name() == "b")
            {
                    auto const& taint = analysis.taint_for(*decl);
                    BOOST_CHECK_EQUAL(taint.size(), sources);
                    BOOST_CHECK(taint[tag_2]);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 4);
}

BOOST_AUTO_TEST_CASE(tuple_to_tuple)
{
    char const* text = R"(
        contract A {
            function g(int x) public returns (int, int) { return (x, x); }
            function f(int x, int y, int z, int w) public returns (int) {
                int a = x;
                int b = y;
                int c = z;
                int d = w;
                while (a != b)
                {
                    (b, c) = (c, d);
                    (c, d) = (d, b);
                }
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 30;
    auto const& func = *ctrt->definedFunctions()[1];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 4;
    size_t tag_2 = 9;
    size_t tag_3 = 15;
    size_t tag_4 = 24;
    set<size_t> x_tag{ tag_1 };
    set<size_t> y_tag{ tag_2 };
    set<size_t> z_tag{ tag_3 };
    set<size_t> w_tag{ tag_4 };
    set<size_t> yzw_tag{ tag_2, tag_3, tag_4 };
    auto const& input_x = (*func.parameters()[0].get());
    auto const& input_y = (*func.parameters()[1].get());
    auto const& input_z = (*func.parameters()[2].get());
    auto const& input_w = (*func.parameters()[3].get());

    analysis.taint(input_x, tag_1);
    taint_check(analysis, input_x, x_tag, sources);
    analysis.taint(input_y, tag_2);
    taint_check(analysis, input_y, y_tag, sources);
    analysis.taint(input_z, tag_3);
    taint_check(analysis, input_z, z_tag, sources);
    analysis.taint(input_w, tag_4);
    taint_check(analysis, input_w, w_tag, sources);

    analysis.run(func);

    taint_check(analysis, input_x, x_tag, sources);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, z_tag, sources);
    taint_check(analysis, input_w, w_tag, sources);

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "a")
            {
                taint_check(analysis, *decl, x_tag, sources);
            }
            else if (decl->name() == "b")
            {
                taint_check(analysis, *decl, yzw_tag, sources);
            }
            else if (decl->name() == "c")
            {
                taint_check(analysis, *decl, yzw_tag, sources);
            }
            else if (decl->name() == "d")
            {
                taint_check(analysis, *decl, yzw_tag, sources);
            }
            else if (decl->name() == "ret")
            {
                taint_check(analysis, *decl, yzw_tag, sources);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 4);
}

BOOST_AUTO_TEST_CASE(fn_to_tuple)
{
    char const* text = R"(
        contract A {
            function g(int x) public returns (int, int) { return (x, x); }
            function f(int x, int y, int z) public returns (int) {
                int a = x;
                int b = y;
                int c = z;
                (a, b) = g(x);
                int ret = a + b;
                return ret;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    size_t sources = 5;
    auto const& func = *ctrt->definedFunctions()[1];
    TaintAnalysis analysis(sources);

    size_t tag_1 = 0;
    size_t tag_2 = 1;
    size_t tag_3 = 2;
    size_t tag_4 = 3;
    size_t tag_5 = 4;
    set<size_t> x_tag{ tag_1 };
    set<size_t> y_tag{ tag_2 };
    set<size_t> z_tag{ tag_3 };
    set<size_t> fn_tag{ tag_1, tag_2, tag_3, tag_4, tag_5 };
    auto const& input_x = (*func.parameters()[0].get());
    auto const& input_y = (*func.parameters()[1].get());
    auto const& input_z = (*func.parameters()[2].get());

    analysis.taint(input_x, tag_1);
    taint_check(analysis, input_x, x_tag, sources);
    analysis.taint(input_y, tag_2);
    taint_check(analysis, input_y, y_tag, sources);
    analysis.taint(input_z, tag_3);
    taint_check(analysis, input_z, z_tag, sources);

    analysis.run(func);

    taint_check(analysis, input_x, x_tag, sources);
    taint_check(analysis, input_y, y_tag, sources);
    taint_check(analysis, input_z, z_tag, sources);

    size_t visited = 0;
    for (auto stmt : func.body().statements())
    {
        auto const* dstmt
            = dynamic_cast<VariableDeclarationStatement const*>(stmt.get());
        if (dstmt)
        {
            auto const *decl = dstmt->declarations()[0].get();
            if (decl->name() == "a")
            {
                taint_check(analysis, *decl, fn_tag, sources);
            }
            else if (decl->name() == "b")
            {
                taint_check(analysis, *decl, fn_tag, sources);
            }
            else if (decl->name() == "c")
            {
                taint_check(analysis, *decl, z_tag, sources);
            }
            else if (decl->name() == "ret")
            {
                taint_check(analysis, *decl, fn_tag, sources);
            }
            visited += 1;
        }
    }
    BOOST_CHECK_EQUAL(visited, 4);
}

BOOST_AUTO_TEST_SUITE_END()

// -------------------------------------------------------------------------- //

}
}
}
}
