/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/FunctionChecker.h
 */

#include <libsolidity/modelcheck/analysis/Mapping.h>

#include <boost/test/unit_test.hpp>
#include <test/libsolidity/AnalysisFramework.h>

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

BOOST_FIXTURE_TEST_SUITE(
    MapDeflateTests,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(basic_lookup)
{
    char const* text = R"(
        contract A {
            struct S { uint val; }
            mapping(uint256 => address) a;
            mapping(int256 => mapping(address => mapping(bool => S))) b;
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");

    MapDeflate lookup;

    for (unsigned int i = 0; i < 5; ++i)
    {
        for (auto sv : ctrt.stateVariables())
        {
            Mapping const* m = dynamic_cast<Mapping const*>(sv->typeName());
            BOOST_CHECK_NE(m, nullptr);
            
            auto const& res = lookup.query(*m);

            if (sv->name() == "a")
            {
                BOOST_CHECK(!res.name.empty());
                BOOST_CHECK_EQUAL(res.key_types.size(), 1);
                BOOST_CHECK(
                    res.value_type->annotation().type->category()
                    ==
                    Type::Category::Address
                );
            }
            else if (sv->name() == "b")
            {
                BOOST_CHECK(!res.name.empty());
                BOOST_CHECK_EQUAL(res.key_types.size(), 3);
                BOOST_CHECK(
                    res.key_types.front()->annotation().type->category()
                    ==
                    Type::Category::Integer
                );
                BOOST_CHECK(
                    res.key_types.back()->annotation().type->category()
                    ==
                    Type::Category::Bool
                );
                BOOST_CHECK(
                    res.value_type->annotation().type->category()
                    ==
                    Type::Category::Struct
                );
            }
            else
            {
                BOOST_CHECK(false);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(basic_access)
{
    char const* text = R"(
        contract A {
            int256 a;
            address b;
            mapping(int256 => mapping(address => mapping(bool => int256))) m;
            function f() public view {
                m[a][b][true];
            }
        }
    )";

    const auto& unit = *parseAndAnalyse(text);
    const auto& ctrt = *retrieveContractByName(unit, "A");
    const auto& mapping = *ctrt.stateVariables()[2];
    const auto& funcdef = *ctrt.definedFunctions()[0];
    const auto& exprstmt = *static_cast<ExpressionStatement const*>(
        funcdef.body().statements()[0].get()
    );
    const auto& mapindex = static_cast<IndexAccess const&>(exprstmt.expression());

    FlatIndex idx1(mapindex);
    FlatIndex idx2(static_cast<IndexAccess const&>(mapindex.baseExpression()));

    BOOST_CHECK_EQUAL(idx1.indices().size(), 3);
    BOOST_CHECK_EQUAL(idx2.indices().size(), 2);
    BOOST_CHECK_EQUAL(&idx1.base(), &idx2.base());
    BOOST_CHECK_EQUAL(&idx1.decl(), &idx2.decl());

    BOOST_CHECK(
        idx1.indices().front()->annotation().type->category()
        ==
        Type::Category::Integer
    );
    BOOST_CHECK(
        idx1.indices().back()->annotation().type->category()
        ==
        Type::Category::Bool
    );
    BOOST_CHECK(
        idx2.indices().front()->annotation().type->category()
        ==
        Type::Category::Integer
    );
    BOOST_CHECK(
        idx2.indices().back()->annotation().type->category()
        ==
        Type::Category::Address
    );
    
    const auto* base = dynamic_cast<Identifier const&>(
        idx1.base()
    ).annotation().referencedDeclaration;
    BOOST_CHECK_EQUAL(base, &mapping);

    BOOST_CHECK_EQUAL(mapping.typeName(), idx1.decl().typeName());
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}