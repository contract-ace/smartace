/**
 * Specific tests for libsolidity/modelcheck/utils/Types.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/utils/Types.h>

#include <boost/test/unit_test.hpp>

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

namespace
{

static EnumDefinition const ENUM_DEF(langutil::SourceLocation(), nullptr, {});
static ContractDefinition const CTRX_DEF(
    langutil::SourceLocation(), nullptr, nullptr, {}, {}
);
static StructDefinition const STRUCT_DEF(
    langutil::SourceLocation(), nullptr, {}
);
static ModifierDefinition const MOD_DEF(
    langutil::SourceLocation(),
    nullptr,
    nullptr,
    make_shared<ParameterList>(
        langutil::SourceLocation(), vector<ASTPointer<VariableDeclaration>>{}
    ),
    nullptr
);

static AddressType const ADDR_TYPE(StateMutability::Payable);
static IntegerType const INT_TYPE(40, IntegerType::Modifier::Signed);
static IntegerType const UINT_TYPE(48, IntegerType::Modifier::Unsigned);
static RationalNumberType const NATURAL_TYPE(256);
static RationalNumberType const RATIONAL_TYPE(rational(1) / rational(2));
static BoolType const BOOL_TYPE;
static EnumType const ENUM_TYPE(ENUM_DEF);
static FixedPointType const FIXED_PT_TYPE(
    32, 10, FixedPointType::Modifier::Signed
);
static FixedPointType const UFIXED_PT_TYPE(
    48, 10, FixedPointType::Modifier::Unsigned
);
static TupleType const TUPLE_TYPE;
static TypeType const SIMPLE_TYPE_TYPE(&BOOL_TYPE);
static TypeType const COMPLEX_TYPE_TYPE(&TUPLE_TYPE);
static TypeType const SIMPLE_TYPE_TYPE_TYPE(&SIMPLE_TYPE_TYPE);
static TypeType const COMPLEX_TYPE_TYPE_TYPE(&COMPLEX_TYPE_TYPE);
static StringLiteralType const STRING_TYPE("blah");
static FixedBytesType const FIXED_BYTE_TYPE(32);
static ArrayType const ARRAY_TYPE(DataLocation::Memory);
static ContractType const CTRX_TYPE(CTRX_DEF);
static StructType const STRUCT_TYPE(STRUCT_DEF);
static MappingType const MAPPING_TYPE(nullptr, nullptr);
static ModifierType const MODIFIER_TYPE(MOD_DEF);
static MagicType const BLOCK_TYPE(MagicType::Kind::Block);
static MagicType const MESSAGE_TYPE(MagicType::Kind::Message);

}

// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(Utils_TypesTests)

BOOST_AUTO_TEST_CASE(unwrap_on_types)
{
    BOOST_CHECK_EQUAL(&unwrap(ADDR_TYPE), &ADDR_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(INT_TYPE), &INT_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(NATURAL_TYPE), NATURAL_TYPE.integerType());
    BOOST_CHECK_EQUAL(&unwrap(RATIONAL_TYPE), RATIONAL_TYPE.fixedPointType());
    BOOST_CHECK_EQUAL(&unwrap(BOOL_TYPE), &BOOL_TYPE);
    BOOST_CHECK_NE(
        static_cast<IntegerType const*>(&unwrap(ENUM_TYPE)),
        nullptr)
    ;
    BOOST_CHECK_EQUAL(&unwrap(FIXED_PT_TYPE), &FIXED_PT_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(SIMPLE_TYPE_TYPE), SIMPLE_TYPE_TYPE.actualType());
    BOOST_CHECK_EQUAL(
        &unwrap(COMPLEX_TYPE_TYPE),
        COMPLEX_TYPE_TYPE.actualType()
    );
    BOOST_CHECK_EQUAL(
        &unwrap(SIMPLE_TYPE_TYPE_TYPE),
        &unwrap(*SIMPLE_TYPE_TYPE_TYPE.actualType())
    );
    BOOST_CHECK_EQUAL(
        &unwrap(COMPLEX_TYPE_TYPE_TYPE),
        &unwrap(*COMPLEX_TYPE_TYPE_TYPE.actualType())
    );
    BOOST_CHECK_EQUAL(&unwrap(STRING_TYPE), &STRING_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(FIXED_BYTE_TYPE), &FIXED_BYTE_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(ARRAY_TYPE), &ARRAY_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(CTRX_TYPE), &CTRX_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(STRUCT_TYPE), &STRUCT_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(TUPLE_TYPE), &TUPLE_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(MAPPING_TYPE), &MAPPING_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(MODIFIER_TYPE), &MODIFIER_TYPE);
    BOOST_CHECK_EQUAL(&unwrap(BLOCK_TYPE), &BLOCK_TYPE);
}

BOOST_AUTO_TEST_CASE(is_basic_on_types)
{
    BOOST_CHECK(is_simple_type(ADDR_TYPE));
    BOOST_CHECK(is_simple_type(INT_TYPE));
    BOOST_CHECK(is_simple_type(NATURAL_TYPE));
    BOOST_CHECK(is_simple_type(RATIONAL_TYPE));
    BOOST_CHECK(is_simple_type(BOOL_TYPE));
    BOOST_CHECK(is_simple_type(ENUM_TYPE));
    BOOST_CHECK(is_simple_type(FIXED_PT_TYPE));
    BOOST_CHECK(!is_simple_type(TUPLE_TYPE));
    BOOST_CHECK(is_simple_type(SIMPLE_TYPE_TYPE));
    BOOST_CHECK(!is_simple_type(COMPLEX_TYPE_TYPE));
    BOOST_CHECK(is_simple_type(SIMPLE_TYPE_TYPE_TYPE));
    BOOST_CHECK(!is_simple_type(COMPLEX_TYPE_TYPE_TYPE));
    BOOST_CHECK(!is_simple_type(STRING_TYPE));
    BOOST_CHECK(!is_simple_type(FIXED_BYTE_TYPE));
    BOOST_CHECK(!is_simple_type(ARRAY_TYPE));
    BOOST_CHECK(!is_simple_type(CTRX_TYPE));
    BOOST_CHECK(!is_simple_type(*CTRX_TYPE.newExpressionType()));
    BOOST_CHECK(!is_simple_type(STRUCT_TYPE));
    BOOST_CHECK(!is_simple_type(MAPPING_TYPE));
    BOOST_CHECK(!is_simple_type(MODIFIER_TYPE));
    BOOST_CHECK(!is_simple_type(BLOCK_TYPE));
}

BOOST_AUTO_TEST_CASE(bits_on_types)
{
    BOOST_CHECK_EQUAL(simple_bit_count(ADDR_TYPE), 160);
    BOOST_CHECK_EQUAL(simple_bit_count(INT_TYPE), INT_TYPE.numBits());
    BOOST_CHECK_EQUAL(simple_bit_count(UINT_TYPE), UINT_TYPE.numBits());
    BOOST_CHECK_EQUAL(
        simple_bit_count(NATURAL_TYPE),
        NATURAL_TYPE.integerType()->numBits()
    );
    BOOST_CHECK_EQUAL(
        simple_bit_count(RATIONAL_TYPE),
        RATIONAL_TYPE.fixedPointType()->numBits()
    );
    BOOST_CHECK_EQUAL(simple_bit_count(BOOL_TYPE), 8);
    BOOST_CHECK_EQUAL(simple_bit_count(FIXED_PT_TYPE), FIXED_PT_TYPE.numBits());
    BOOST_CHECK_EQUAL(simple_bit_count(UFIXED_PT_TYPE), UFIXED_PT_TYPE.numBits());
}

BOOST_AUTO_TEST_CASE(signedness_on_types)
{
    BOOST_CHECK(!simple_is_signed(ADDR_TYPE));
    BOOST_CHECK(simple_is_signed(INT_TYPE));
    BOOST_CHECK(!simple_is_signed(UINT_TYPE));
    BOOST_CHECK(!simple_is_signed(NATURAL_TYPE));
    BOOST_CHECK(!simple_is_signed(RATIONAL_TYPE));
    BOOST_CHECK(!simple_is_signed(BOOL_TYPE));
    BOOST_CHECK(simple_is_signed(FIXED_PT_TYPE));
    BOOST_CHECK(!simple_is_signed(UFIXED_PT_TYPE));
}

BOOST_AUTO_TEST_CASE(is_wrapped_on_types)
{
    BOOST_CHECK(is_wrapped_type(ADDR_TYPE));
    BOOST_CHECK(is_wrapped_type(INT_TYPE));
    BOOST_CHECK(is_wrapped_type(UINT_TYPE));
    BOOST_CHECK(is_wrapped_type(NATURAL_TYPE));
    BOOST_CHECK(is_wrapped_type(RATIONAL_TYPE));
    BOOST_CHECK(is_wrapped_type(BOOL_TYPE));
    BOOST_CHECK(is_wrapped_type(FIXED_PT_TYPE));
    BOOST_CHECK(is_wrapped_type(UFIXED_PT_TYPE));
    BOOST_CHECK(is_wrapped_type(ENUM_TYPE));
    BOOST_CHECK(!is_wrapped_type(*CTRX_TYPE.newExpressionType()));
    BOOST_CHECK(!is_wrapped_type(STRUCT_TYPE));
    BOOST_CHECK(!is_wrapped_type(MAPPING_TYPE));
    BOOST_CHECK(!is_wrapped_type(MODIFIER_TYPE));
    BOOST_CHECK(!is_wrapped_type(BLOCK_TYPE));
}

BOOST_AUTO_TEST_CASE(name_escaping)
{
    using langutil::SourceLocation;

    vector<pair<shared_ptr<string>, string>> cases{
        { make_shared<string>("aa"), "aa" },
        { make_shared<string>("aa_"), "aa__" },
        { make_shared<string>("aa_bb_"), "aa__bb__" },
        { make_shared<string>("aa__bb"), "aa____bb" }
    };

    for (auto const& c : cases)
    {
        ContractDefinition def(SourceLocation(), c.first, nullptr, {}, {});

        string escaped_name = escape_decl_name(def);
        BOOST_CHECK_EQUAL(escaped_name, c.second);
        BOOST_CHECK_EQUAL(escaped_name, escape_decl_name_string(def.name()));
    }
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
