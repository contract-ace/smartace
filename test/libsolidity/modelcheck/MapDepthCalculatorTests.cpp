/**
 * @date 2019
 * Tests maps of various depth with the MapDepthCalculator visitor. Complex
 * value types are also tested.
 */

#include <libsolidity/modelcheck/MapDepthCalculator.h>

#include <test/libsolidity/AnalysisFramework.h>
#include <boost/test/unit_test.hpp>

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{

#define DO_MAP_DEPTH_TEST(src_text, expt_val) \
do { \
    SourceUnit const* ast = parseAndAnalyse(src_text); \
    ContractDefinition const* c = retrieveContractByName(*ast, "A"); \
    BOOST_CHECK_NE(c, nullptr); \
    BOOST_CHECK_NE(c->stateVariables().size(), 0); \
    auto d = c->stateVariables()[0]; \
    auto m = dynamic_cast<Mapping const*>(d->typeName()); \
    BOOST_CHECK_NE(m, nullptr); \
    MapDepthCalculator calc(*m); \
    BOOST_CHECK_EQUAL(calc.depth(), expt_val); \
    BOOST_CHECK_EQUAL(calc.depth(), expt_val); \
} while(0)

BOOST_FIXTURE_TEST_SUITE(MapDepth, ::dev::solidity::test::AnalysisFramework);

BOOST_AUTO_TEST_CASE(depth_1_test)
{
    char const* text = R"(
		contract A {
			mapping (uint => uint) a;
		}
	)";
    DO_MAP_DEPTH_TEST(text, 1);
}

BOOST_AUTO_TEST_CASE(depth_2_test)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => uint)) a;
		}
	)";
    DO_MAP_DEPTH_TEST(text, 2);
}

BOOST_AUTO_TEST_CASE(depth_3_test)
{
    char const* text = R"(
		contract A {
			mapping (uint => mapping (uint => mapping (uint => uint))) a;
		}
	)";
    DO_MAP_DEPTH_TEST(text, 3);
}

BOOST_AUTO_TEST_CASE(complex_type)
{
    char const* text = R"(
		contract A {
            struct A {
                uint a;
            }
			mapping (uint => A) a;
		}
	)";
    DO_MAP_DEPTH_TEST(text, 1);
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
