/**
 * @date 2019
 * Specific tests for libsolidity/modelcheck/TypeClassification.h
 */

#include <libsolidity/modelcheck/TypeClassification.h>

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

BOOST_FIXTURE_TEST_SUITE(
    TypeClassificationTests,
    ::dev::solidity::test::AnalysisFramework
)

BOOST_AUTO_TEST_CASE(is_basic_on_types)
{
    using langutil::SourceLocation;

    auto params = make_shared<ParameterList>(
        SourceLocation(), vector<ASTPointer<VariableDeclaration>>{}
    );
    EnumDefinition enum_def(SourceLocation(), nullptr, {});
    ContractDefinition ctrx_def(SourceLocation(), nullptr, nullptr, {}, {});
    StructDefinition struct_def(SourceLocation(), nullptr, {});
    ModifierDefinition mod_def(
        SourceLocation(), nullptr, nullptr, params, nullptr
    );

    BoolType bool_type;
    TupleType tuple_type;
    ContractType ctrx_type(ctrx_def);

    BOOST_CHECK(is_simple_type(AddressType(StateMutability::Payable)));
    BOOST_CHECK(is_simple_type(IntegerType(32)));
    BOOST_CHECK(is_simple_type(RationalNumberType(256)));
    BOOST_CHECK(is_simple_type(bool_type));
    BOOST_CHECK(is_simple_type(EnumType(enum_def)));
    BOOST_CHECK(is_simple_type(FixedPointType(32, 10)));
    BOOST_CHECK(is_simple_type(TypeType(&bool_type)));
    BOOST_CHECK(!is_simple_type(StringLiteralType("blah")));
    BOOST_CHECK(!is_simple_type(FixedBytesType(32)));
    BOOST_CHECK(!is_simple_type(ArrayType(DataLocation::Memory)));
    BOOST_CHECK(!is_simple_type(ctrx_type));
    BOOST_CHECK(!is_simple_type(StructType(struct_def)));
    BOOST_CHECK(!is_simple_type(tuple_type));
    BOOST_CHECK(!is_simple_type(*ctrx_type.newExpressionType()));
    BOOST_CHECK(!is_simple_type(MappingType(nullptr, nullptr)));
    BOOST_CHECK(!is_simple_type(ModifierType(mod_def)));
    BOOST_CHECK(!is_simple_type(MagicType(nullptr)));
    BOOST_CHECK(!is_simple_type(TypeType(&tuple_type)));
}

BOOST_AUTO_TEST_SUITE_END();

}
}
}
}
