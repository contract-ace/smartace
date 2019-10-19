/**
 * Defines assert and require implementations for static analysis.
 * TODO(scottwe): sol_assert/sol_require may require more complicated behaviour
 *                later on... it would make sense for each to call into a "ll"
 *                implementation.
 * @date 2019
 */

#include "verify.h"

#include "seahorn/seahorn.h"

// -------------------------------------------------------------------------- //

void sol_setup(int _argc, const char **_argv) {}

// -------------------------------------------------------------------------- //

void sol_on_transaction(void) {}

// -------------------------------------------------------------------------- //

void ll_assume(sol_raw_uint8_t _cond)
{
    assume(_cond);
}

// -------------------------------------------------------------------------- //

void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    sassert(_cond);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
    (void) _msg;
    ll_assume(_cond);
}

// -------------------------------------------------------------------------- //
extern sol_raw_uint8_t sea_nd_u8(void);
sol_raw_uint8_t nd_uint8_t(const char* _msg)
{
	sol_raw_int8_t v = sea_nd_u8();
	assume(v >= 0);
	return v;
}

extern sol_raw_int8_t sea_nd_i8(void);
sol_raw_int8_t nd_int8_t(const char* _msg)
{
	return sea_nd_i8();
}

extern sol_raw_uint16_t sea_nd_u16(void);
sol_raw_uint16_t nd_uint16_t(const char* _msg)
{
	sol_raw_int16_t v = sea_nd_u16();
	assume(v >= 0);
	return v;
}

extern sol_raw_int16_t sea_nd_i16(void);
sol_raw_int16_t nd_int16_t(const char* _msg)
{
	return sea_nd_i16();
}

extern sol_raw_uint24_t sea_nd_u24(void);
sol_raw_uint24_t nd_uint24_t(const char* _msg)
{
	sol_raw_int24_t v = sea_nd_u24();
	assume(v >= 0);
	return v;
}

extern sol_raw_int24_t sea_nd_i24(void);
sol_raw_int24_t nd_int24_t(const char* _msg)
{
	return sea_nd_i24();
}

extern sol_raw_uint32_t sea_nd_u32(void);
sol_raw_uint32_t nd_uint32_t(const char* _msg)
{
	sol_raw_int32_t v = sea_nd_u32();
	assume(v >= 0);
	return v;
}

extern sol_raw_int32_t sea_nd_i32(void);
sol_raw_int32_t nd_int32_t(const char* _msg)
{
	return sea_nd_i32();
}

extern sol_raw_uint40_t sea_nd_u40(void);
sol_raw_uint40_t nd_uint40_t(const char* _msg)
{
	sol_raw_int40_t v = sea_nd_u40();
	assume(v >= 0);
	return v;
}

extern sol_raw_int40_t sea_nd_i40(void);
sol_raw_int40_t nd_int40_t(const char* _msg)
{
	return sea_nd_i40();
}

extern sol_raw_uint48_t sea_nd_u48(void);
sol_raw_uint48_t nd_uint48_t(const char* _msg)
{
	sol_raw_int48_t v = sea_nd_u48();
	assume(v >= 0);
	return v;
}

extern sol_raw_int48_t sea_nd_i48(void);
sol_raw_int48_t nd_int48_t(const char* _msg)
{
	return sea_nd_i48();
}

extern sol_raw_uint56_t sea_nd_u56(void);
sol_raw_uint56_t nd_uint56_t(const char* _msg)
{
	sol_raw_int56_t v = sea_nd_u56();
	assume(v >= 0);
	return v;
}

extern sol_raw_int56_t sea_nd_i56(void);
sol_raw_int56_t nd_int56_t(const char* _msg)
{
	return sea_nd_i56();
}

extern sol_raw_uint64_t sea_nd_u64(void);
sol_raw_uint64_t nd_uint64_t(const char* _msg)
{
	sol_raw_int64_t v = sea_nd_u64();
	assume(v >= 0);
	return v;
}

extern sol_raw_int64_t sea_nd_i64(void);
sol_raw_int64_t nd_int64_t(const char* _msg)
{
	return sea_nd_i64();
}

extern sol_raw_uint72_t sea_nd_u72(void);
sol_raw_uint72_t nd_uint72_t(const char* _msg)
{
	sol_raw_int72_t v = sea_nd_u72();
	assume(v >= 0);
	return v;
}

extern sol_raw_int72_t sea_nd_i72(void);
sol_raw_int72_t nd_int72_t(const char* _msg)
{
	return sea_nd_i72();
}

extern sol_raw_uint80_t sea_nd_u80(void);
sol_raw_uint80_t nd_uint80_t(const char* _msg)
{
	sol_raw_int80_t v = sea_nd_u80();
	assume(v >= 0);
	return v;
}

extern sol_raw_int80_t sea_nd_i80(void);
sol_raw_int80_t nd_int80_t(const char* _msg)
{
	return sea_nd_i80();
}

extern sol_raw_uint88_t sea_nd_u88(void);
sol_raw_uint88_t nd_uint88_t(const char* _msg)
{
	sol_raw_int88_t v = sea_nd_u88();
	assume(v >= 0);
	return v;
}

extern sol_raw_int88_t sea_nd_i88(void);
sol_raw_int88_t nd_int88_t(const char* _msg)
{
	return sea_nd_i88();
}

extern sol_raw_uint96_t sea_nd_u96(void);
sol_raw_uint96_t nd_uint96_t(const char* _msg)
{
	sol_raw_int96_t v = sea_nd_u96();
	assume(v >= 0);
	return v;
}

extern sol_raw_int96_t sea_nd_i96(void);
sol_raw_int96_t nd_int96_t(const char* _msg)
{
	return sea_nd_i96();
}

extern sol_raw_uint104_t sea_nd_u104(void);
sol_raw_uint104_t nd_uint104_t(const char* _msg)
{
	sol_raw_int104_t v = sea_nd_u104();
	assume(v >= 0);
	return v;
}

extern sol_raw_int104_t sea_nd_i104(void);
sol_raw_int104_t nd_int104_t(const char* _msg)
{
	return sea_nd_i104();
}

extern sol_raw_uint112_t sea_nd_u112(void);
sol_raw_uint112_t nd_uint112_t(const char* _msg)
{
	sol_raw_int112_t v = sea_nd_u112();
	assume(v >= 0);
	return v;
}

extern sol_raw_int112_t sea_nd_i112(void);
sol_raw_int112_t nd_int112_t(const char* _msg)
{
	return sea_nd_i112();
}

extern sol_raw_uint120_t sea_nd_u120(void);
sol_raw_uint120_t nd_uint120_t(const char* _msg)
{
	sol_raw_int120_t v = sea_nd_u120();
	assume(v >= 0);
	return v;
}

extern sol_raw_int120_t sea_nd_i120(void);
sol_raw_int120_t nd_int120_t(const char* _msg)
{
	return sea_nd_i120();
}

extern sol_raw_uint128_t sea_nd_u128(void);
sol_raw_uint128_t nd_uint128_t(const char* _msg)
{
	sol_raw_int128_t v = sea_nd_u128();
	assume(v >= 0);
	return v;
}

extern sol_raw_int128_t sea_nd_i128(void);
sol_raw_int128_t nd_int128_t(const char* _msg)
{
	return sea_nd_i128();
}

extern sol_raw_uint136_t sea_nd_u136(void);
sol_raw_uint136_t nd_uint136_t(const char* _msg)
{
	sol_raw_int136_t v = sea_nd_u136();
	assume(v >= 0);
	return v;
}

extern sol_raw_int136_t sea_nd_i136(void);
sol_raw_int136_t nd_int136_t(const char* _msg)
{
	return sea_nd_i136();
}

extern sol_raw_uint144_t sea_nd_u144(void);
sol_raw_uint144_t nd_uint144_t(const char* _msg)
{
	sol_raw_int144_t v = sea_nd_u144();
	assume(v >= 0);
	return v;
}

extern sol_raw_int144_t sea_nd_i144(void);
sol_raw_int144_t nd_int144_t(const char* _msg)
{
	return sea_nd_i144();
}

extern sol_raw_uint152_t sea_nd_u152(void);
sol_raw_uint152_t nd_uint152_t(const char* _msg)
{
	sol_raw_int152_t v = sea_nd_u152();
	assume(v >= 0);
	return v;
}

extern sol_raw_int152_t sea_nd_i152(void);
sol_raw_int152_t nd_int152_t(const char* _msg)
{
	return sea_nd_i152();
}

extern sol_raw_uint160_t sea_nd_u160(void);
sol_raw_uint160_t nd_uint160_t(const char* _msg)
{
	sol_raw_int160_t v = sea_nd_u160();
	assume(v >= 0);
	return v;
}

extern sol_raw_int160_t sea_nd_i160(void);
sol_raw_int160_t nd_int160_t(const char* _msg)
{
	return sea_nd_i160();
}

extern sol_raw_uint168_t sea_nd_u168(void);
sol_raw_uint168_t nd_uint168_t(const char* _msg)
{
	sol_raw_int168_t v = sea_nd_u168();
	assume(v >= 0);
	return v;
}

extern sol_raw_int168_t sea_nd_i168(void);
sol_raw_int168_t nd_int168_t(const char* _msg)
{
	return sea_nd_i168();
}

extern sol_raw_uint176_t sea_nd_u176(void);
sol_raw_uint176_t nd_uint176_t(const char* _msg)
{
	sol_raw_int176_t v = sea_nd_u176();
	assume(v >= 0);
	return v;
}

extern sol_raw_int176_t sea_nd_i176(void);
sol_raw_int176_t nd_int176_t(const char* _msg)
{
	return sea_nd_i176();
}

extern sol_raw_uint184_t sea_nd_u184(void);
sol_raw_uint184_t nd_uint184_t(const char* _msg)
{
	sol_raw_int184_t v = sea_nd_u184();
	assume(v >= 0);
	return v;
}

extern sol_raw_int184_t sea_nd_i184(void);
sol_raw_int184_t nd_int184_t(const char* _msg)
{
	return sea_nd_i184();
}

extern sol_raw_uint192_t sea_nd_u192(void);
sol_raw_uint192_t nd_uint192_t(const char* _msg)
{
	sol_raw_int192_t v = sea_nd_u192();
	assume(v >= 0);
	return v;
}

extern sol_raw_int192_t sea_nd_i192(void);
sol_raw_int192_t nd_int192_t(const char* _msg)
{
	return sea_nd_i192();
}

extern sol_raw_uint200_t sea_nd_u200(void);
sol_raw_uint200_t nd_uint200_t(const char* _msg)
{
	sol_raw_int200_t v = sea_nd_u200();
	assume(v >= 0);
	return v;
}

extern sol_raw_int200_t sea_nd_i200(void);
sol_raw_int200_t nd_int200_t(const char* _msg)
{
	return sea_nd_i200();
}

extern sol_raw_uint208_t sea_nd_u208(void);
sol_raw_uint208_t nd_uint208_t(const char* _msg)
{
	sol_raw_int208_t v = sea_nd_u208();
	assume(v >= 0);
	return v;
}

extern sol_raw_int208_t sea_nd_i208(void);
sol_raw_int208_t nd_int208_t(const char* _msg)
{
	return sea_nd_i208();
}

extern sol_raw_uint216_t sea_nd_u216(void);
sol_raw_uint216_t nd_uint216_t(const char* _msg)
{
	sol_raw_int216_t v = sea_nd_u216();
	assume(v >= 0);
	return v;
}

extern sol_raw_int216_t sea_nd_i216(void);
sol_raw_int216_t nd_int216_t(const char* _msg)
{
	return sea_nd_i216();
}

extern sol_raw_uint224_t sea_nd_u224(void);
sol_raw_uint224_t nd_uint224_t(const char* _msg)
{
	sol_raw_int224_t v = sea_nd_u224();
	assume(v >= 0);
	return v;
}

extern sol_raw_int224_t sea_nd_i224(void);
sol_raw_int224_t nd_int224_t(const char* _msg)
{
	return sea_nd_i224();
}

extern sol_raw_uint232_t sea_nd_u232(void);
sol_raw_uint232_t nd_uint232_t(const char* _msg)
{
	sol_raw_int232_t v = sea_nd_u232();
	assume(v >= 0);
	return v;
}

extern sol_raw_int232_t sea_nd_i232(void);
sol_raw_int232_t nd_int232_t(const char* _msg)
{
	return sea_nd_i232();
}

extern sol_raw_uint240_t sea_nd_u240(void);
sol_raw_uint240_t nd_uint240_t(const char* _msg)
{
	sol_raw_int240_t v = sea_nd_u240();
	assume(v >= 0);
	return v;
}

extern sol_raw_int240_t sea_nd_i240(void);
sol_raw_int240_t nd_int240_t(const char* _msg)
{
	return sea_nd_i240();
}

extern sol_raw_uint248_t sea_nd_u248(void);
sol_raw_uint248_t nd_uint248_t(const char* _msg)
{
	sol_raw_int248_t v = sea_nd_u248();
	assume(v >= 0);
	return v;
}

extern sol_raw_int248_t sea_nd_i248(void);
sol_raw_int248_t nd_int248_t(const char* _msg)
{
	return sea_nd_i248();
}

extern sol_raw_uint256_t sea_nd_u256(void);
sol_raw_uint256_t nd_uint256_t(const char* _msg)
{
	sol_raw_int256_t v = sea_nd_u256();
	assume(v >= 0);
	return v;
}

extern sol_raw_int256_t sea_nd_i256(void);
sol_raw_int256_t nd_int256_t(const char* _msg)
{
	return sea_nd_i256();
}

// -------------------------------------------------------------------------- //
