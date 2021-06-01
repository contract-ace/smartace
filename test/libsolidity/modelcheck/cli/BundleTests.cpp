/**
 * Specific tests for libsolidity/modelcheck/cli/Bundle.
 * 
 * @date 2021
 */

#include <libsolidity/modelcheck/cli/Bundle.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

#include <libsolidity/ast/AST.h>

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
    Cli_BundleTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(single_extraction)
{
    char const* text = R"(
        contract A { }
        contract B { }
        contract C { }
        contract D { }
        contract E { }
        contract F { }
        interface I { function f() external view returns (uint); }
        library L { function f() public {} }
    )";

    auto const unit = parseAndAnalyse(text);

    vector<SourceUnit const*> full({ unit });
    vector<string> names({ "B" });
    BundleExtractor extractor(full, names);

    auto const& model = extractor.get();
    BOOST_CHECK_EQUAL(model.size(), 1);
    if (model.size() == 1)
    {
        BOOST_CHECK_EQUAL(model[0]->name(), names[0]);
    }
}

BOOST_AUTO_TEST_CASE(multi_extraction)
{
    char const* text = R"(
        contract A { }
        contract B { }
        contract C { }
        contract D { }
        contract E { }
        contract F { }
        interface I { function f() external view returns (uint); }
        library L { function f() public {} }
    )";

    auto const unit = parseAndAnalyse(text);

    vector<SourceUnit const*> full({ unit });
    vector<string> names({ "B", "D", "F" });
    BundleExtractor extractor(full, names);

    auto const& model = extractor.get();
    BOOST_CHECK_EQUAL(model.size(), 3);
    if (model.size() == 3)
    {
        BOOST_CHECK_EQUAL(model[0]->name(), names[0]);
        BOOST_CHECK_EQUAL(model[1]->name(), names[1]);
        BOOST_CHECK_EQUAL(model[2]->name(), names[2]);
    }
}

BOOST_AUTO_TEST_CASE(missing_name)
{
    char const* text = R"(
        contract A { }
        contract B { }
        contract C { }
        contract D { }
        contract E { }
        contract F { }
        interface I { function f() external view returns (uint); }
        library L { function f() public {} }
    )";

    auto const unit = parseAndAnalyse(text);

    vector<SourceUnit const*> full({ unit });
    vector<string> names({ "B", "Missing1", "D", "Missing2", "F" });
    BundleExtractor extractor(full, names);

    auto const& model = extractor.get();
    BOOST_CHECK_EQUAL(model.size(), 3);
    if (model.size() == 3)
    {
        BOOST_CHECK_EQUAL(model[0]->name(), names[0]);
        BOOST_CHECK_EQUAL(model[1]->name(), names[2]);
        BOOST_CHECK_EQUAL(model[2]->name(), names[4]);
    }

    auto const& missing = extractor.missing();
    BOOST_CHECK_EQUAL(missing.size(), 2);
    if (missing.size() == 2)
    {
        BOOST_CHECK_EQUAL(missing[0], names[1]);
        BOOST_CHECK_EQUAL(missing[1], names[3]);
    }
}

BOOST_AUTO_TEST_CASE(interfaces)
{
    char const* text = R"(
        contract A { }
        contract B { }
        contract C { }
        contract D { }
        contract E { }
        contract F { }
        interface I { function f() external view returns (uint); }
        library L { function f() public {} }
    )";

    auto const unit = parseAndAnalyse(text);

    vector<SourceUnit const*> full({ unit });
    vector<string> names({ "I" });
    BundleExtractor extractor(full, names);

    BOOST_CHECK(extractor.get().empty());
    BOOST_CHECK_EQUAL(extractor.missing().size(), 1);
}

BOOST_AUTO_TEST_CASE(libraries)
{
    char const* text = R"(
        contract A { }
        contract B { }
        contract C { }
        contract D { }
        contract E { }
        contract F { }
        interface I { function f() external view returns (uint); }
        library L { function f() public {} }
    )";

    auto const unit = parseAndAnalyse(text);

    vector<SourceUnit const*> full({ unit });
    vector<string> names({ "L" });
    BundleExtractor extractor(full, names);

    BOOST_CHECK(extractor.get().empty());
    BOOST_CHECK_EQUAL(extractor.missing().size(), 1);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}