/**
 * Specific tests for libsolidity/modelcheck/analysis/Structure.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/Structure.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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

// -------------------------------------------------------------------------- //

BOOST_FIXTURE_TEST_SUITE(
    Analysis_StructureTests, ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(structure)
{
    char const* text = R"(
        contract A {
            struct B {
                int a;
            }
            struct C {
                int a;
                int b;
                mapping(address => uint) c;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    Structure structure_b(*ctrt->definedStructs()[0]);
    Structure structure_c(*ctrt->definedStructs()[1]);

    BOOST_CHECK_EQUAL(structure_b.mappings().size(), 0);
    BOOST_CHECK_EQUAL(structure_c.mappings().size(), 1);
    BOOST_CHECK_EQUAL(structure_b.fields().size(), 1);
    BOOST_CHECK_EQUAL(structure_c.fields().size(), 3);
    BOOST_CHECK_EQUAL(structure_b.name(), "B");
    BOOST_CHECK_EQUAL(structure_c.name(), "C");
}

BOOST_AUTO_TEST_CASE(container)
{
    char const* text = R"(
        contract A {
            struct B {
                int a;
            }
            struct Middle {
                int a;
            }
            struct C {
                int a;
                int b;
                mapping(address => uint) c;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");

    auto store = make_shared<StructureStore>();
    StructureContainer container(*ctrt, store);

    BOOST_CHECK_EQUAL(container.name(), "A");
    BOOST_CHECK_EQUAL(container.structures().size(), 3);
    BOOST_CHECK_EQUAL(container.structures().front()->name(), "B");
    BOOST_CHECK_EQUAL(container.structures().back()->name(), "C");
}

BOOST_AUTO_TEST_CASE(container_inheritance)
{
    char const* text = R"(
        contract A {
            struct X {
                int a;
            }
        }
        contract B is A {
            struct Middle {
                int a;
            }
        }
        contract C is B {
            struct Y {
                int a;
                int b;
                mapping(address => uint) c;
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "C");

    auto store = make_shared<StructureStore>();
    StructureContainer container(*ctrt, store);

    BOOST_CHECK_EQUAL(container.structures().size(), 3);
    BOOST_CHECK_EQUAL(container.structures().front()->name(), "X");
    BOOST_CHECK_EQUAL(container.structures().back()->name(), "Y");
}

BOOST_AUTO_TEST_CASE(lib_structures)
{
    char const* text = R"(
        library Lib {
            struct X {
                int a;
            }
        }
        contract A {
            Lib.X x;
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    auto ctrt = retrieveContractByName(unit, "A");
    
    auto const* var = ctrt->stateVariables()[0];
    BOOST_CHECK(var != nullptr);

    auto store = make_shared<StructureStore>();
    StructureContainer container(*ctrt, store);
    BOOST_CHECK(container.find_structure(var) != nullptr);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
