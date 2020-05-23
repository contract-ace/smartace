/**
 * Defines assert and require implementations for static analysis.
 * TODO(scottwe): sol_assert/sol_require may require more complicated behaviour
 *                later on... it would make sense for each to call into a "ll"
 *                implementation.
 * @date 2019
 */

#include "verify.h"

#include "seahorn/seahorn.h"
#include <stdio.h>

// -------------------------------------------------------------------------- //

void sol_setup(int _argc, const char **_argv) {}

// -------------------------------------------------------------------------- //

uint8_t sol_continue(void)
{
	return 1;
}

// -------------------------------------------------------------------------- //

void sol_on_transaction(void) {}

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

void sol_emit(const char* _msg)
{
	(void) _msg;
	#ifdef MC_LOG_ALL
    printf("Emit: %s\n", _msg);
	#endif
}

// -------------------------------------------------------------------------- //

void ll_assume(sol_raw_uint8_t _cond)
{
    assume(_cond);
}

// -------------------------------------------------------------------------- //

#ifdef MC_LOG_ALL
void on_entry(const char* _type, const char* _msg)
{
	printf("%s [%s]: ", _msg, _type);
}
#endif

uint8_t ll_nd_byte(void);
uint8_t nd_byte(const char* _msg)
{
    uint8_t v = ll_nd_byte();
	#ifdef MC_LOG_ALL
	on_entry("uint8", _msg);
    printf("%hhu\n", v);
	#endif
    return v;
}

uint8_t nd_range(uint8_t l, uint8_t u, const char* _msg)
{
	uint8_t v = nd_byte(_msg);
	ll_assume(v >= l);
	ll_assume(v < u);
	return v;
}

// -------------------------------------------------------------------------- //

sol_raw_uint256_t nd_increase(
	sol_raw_uint256_t _curr, uint8_t _strict, const char* _msg
)
{
	sol_raw_uint256_t next = nd_uint256_t(_msg);
	if (_strict) ll_assume(next > _curr);
	else ll_assume(next >= _curr);
	return next;
}

// -------------------------------------------------------------------------- //

void smartace_log(const char* _msg)
{
	#ifdef MC_LOG_ALL
	printf("%s\n", _msg);
	#endif
}

// -------------------------------------------------------------------------- //

extern sol_raw_int8_t sea_nd_u8(void);
sol_raw_uint8_t nd_uint8_t(const char* _msg)
{
	sol_raw_int8_t v = sea_nd_u8();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint8", _msg);
	printf("%hhu\n", v);
	#endif
	return v;
}

extern sol_raw_int8_t sea_nd_i8(void);
sol_raw_int8_t nd_int8_t(const char* _msg)
{
	sol_raw_int8_t v = sea_nd_i8();
	#ifdef MC_LOG_ALL
	on_entry("int8", _msg);
	printf("%hhu\n", v);
	#endif
	return v;
}

extern sol_raw_uint16_t sea_nd_u16(void);
sol_raw_uint16_t nd_uint16_t(const char* _msg)
{
	sol_raw_int16_t v = sea_nd_u16();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint16", _msg);
	printf("%hu\n", v);
	#endif
	return v;
}

extern sol_raw_int16_t sea_nd_i16(void);
sol_raw_int16_t nd_int16_t(const char* _msg)
{
	sol_raw_int16_t v = sea_nd_i16();
	#ifdef MC_LOG_ALL
	on_entry("int16", _msg);
	printf("%hd\n", v);
	#endif
	return v;
}

extern sol_raw_uint24_t sea_nd_u24(void);
sol_raw_uint24_t nd_uint24_t(const char* _msg)
{
	sol_raw_int24_t v = sea_nd_u24();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint24", _msg);
	printf("%u\n", v);
	#endif
	return v;
}

extern sol_raw_int24_t sea_nd_i24(void);
sol_raw_int24_t nd_int24_t(const char* _msg)
{
	sol_raw_int24_t v = sea_nd_i24();
	#ifdef MC_LOG_ALL
	on_entry("int24", _msg);
	printf("%d\n", v);
	#endif
	return v;
}

extern sol_raw_uint32_t sea_nd_u32(void);
sol_raw_uint32_t nd_uint32_t(const char* _msg)
{
	sol_raw_int32_t v = sea_nd_u32();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint32", _msg);
	printf("%u\n", v);
	#endif
	return v;
}

extern sol_raw_int32_t sea_nd_i32(void);
sol_raw_int32_t nd_int32_t(const char* _msg)
{
	sol_raw_int32_t v = sea_nd_i32();
	#ifdef MC_LOG_ALL
	on_entry("int32", _msg);
	printf("%d\n", v);
	#endif
	return v;
}

extern sol_raw_uint40_t sea_nd_u40(void);
sol_raw_uint40_t nd_uint40_t(const char* _msg)
{
	sol_raw_int40_t v = sea_nd_u40();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint40", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int40_t sea_nd_i40(void);
sol_raw_int40_t nd_int40_t(const char* _msg)
{
	sol_raw_int40_t v = sea_nd_i40();
	#ifdef MC_LOG_ALL
	on_entry("int40", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint48_t sea_nd_u48(void);
sol_raw_uint48_t nd_uint48_t(const char* _msg)
{
	sol_raw_int48_t v = sea_nd_u48();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uin48", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int48_t sea_nd_i48(void);
sol_raw_int48_t nd_int48_t(const char* _msg)
{
	sol_raw_int48_t v = sea_nd_i48();
	#ifdef MC_LOG_ALL
	on_entry("int48", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint56_t sea_nd_u56(void);
sol_raw_uint56_t nd_uint56_t(const char* _msg)
{
	sol_raw_int56_t v = sea_nd_u56();
	#ifdef MC_LOG_ALL
	assume(v >= 0);
	on_entry("uint56", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int56_t sea_nd_i56(void);
sol_raw_int56_t nd_int56_t(const char* _msg)
{
	sol_raw_int56_t v = sea_nd_i56();
	#ifdef MC_LOG_ALL
	on_entry("int56", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint64_t sea_nd_u64(void);
sol_raw_uint64_t nd_uint64_t(const char* _msg)
{
	sol_raw_int64_t v = sea_nd_u64();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint64", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int64_t sea_nd_i64(void);
sol_raw_int64_t nd_int64_t(const char* _msg)
{
	sol_raw_int64_t v = sea_nd_i64();
	#ifdef MC_LOG_ALL
	on_entry("int64", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint72_t sea_nd_u72(void);
sol_raw_uint72_t nd_uint72_t(const char* _msg)
{
	sol_raw_int72_t v = sea_nd_u72();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint72", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int72_t sea_nd_i72(void);
sol_raw_int72_t nd_int72_t(const char* _msg)
{
	sol_raw_int72_t v = sea_nd_i72();
	#ifdef MC_LOG_ALL
	on_entry("int72", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint80_t sea_nd_u80(void);
sol_raw_uint80_t nd_uint80_t(const char* _msg)
{
	sol_raw_int80_t v = sea_nd_u80();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint80", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int80_t sea_nd_i80(void);
sol_raw_int80_t nd_int80_t(const char* _msg)
{
	sol_raw_int80_t v = sea_nd_i80();
	#ifdef MC_LOG_ALL
	on_entry("uint80", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint88_t sea_nd_u88(void);
sol_raw_uint88_t nd_uint88_t(const char* _msg)
{
	sol_raw_int88_t v = sea_nd_u88();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint88", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int88_t sea_nd_i88(void);
sol_raw_int88_t nd_int88_t(const char* _msg)
{
	sol_raw_int88_t v = sea_nd_i88();
	#ifdef MC_LOG_ALL
	on_entry("int88", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint96_t sea_nd_u96(void);
sol_raw_uint96_t nd_uint96_t(const char* _msg)
{
	sol_raw_int96_t v = sea_nd_u96();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint96", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int96_t sea_nd_i96(void);
sol_raw_int96_t nd_int96_t(const char* _msg)
{
	sol_raw_int96_t v = sea_nd_i96();
	#ifdef MC_LOG_ALL
	on_entry("int96", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint104_t sea_nd_u104(void);
sol_raw_uint104_t nd_uint104_t(const char* _msg)
{
	sol_raw_int104_t v = sea_nd_u104();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint104", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int104_t sea_nd_i104(void);
sol_raw_int104_t nd_int104_t(const char* _msg)
{
	sol_raw_int104_t v = sea_nd_i104();
	#ifdef MC_LOG_ALL
	on_entry("int104", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint112_t sea_nd_u112(void);
sol_raw_uint112_t nd_uint112_t(const char* _msg)
{
	sol_raw_int112_t v = sea_nd_u112();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint104", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int112_t sea_nd_i112(void);
sol_raw_int112_t nd_int112_t(const char* _msg)
{
	sol_raw_int112_t v = sea_nd_i112();
	#ifdef MC_LOG_ALL
	on_entry("int112", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint120_t sea_nd_u120(void);
sol_raw_uint120_t nd_uint120_t(const char* _msg)
{
	sol_raw_int120_t v = sea_nd_u120();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint112", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int120_t sea_nd_i120(void);
sol_raw_int120_t nd_int120_t(const char* _msg)
{
	sol_raw_int120_t v = sea_nd_i120();
	#ifdef MC_LOG_ALL
	on_entry("int120", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint128_t sea_nd_u128(void);
sol_raw_uint128_t nd_uint128_t(const char* _msg)
{
	sol_raw_int128_t v = sea_nd_u128();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint120", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int128_t sea_nd_i128(void);
sol_raw_int128_t nd_int128_t(const char* _msg)
{
	sol_raw_int128_t v = sea_nd_i128();
	#ifdef MC_LOG_ALL
	on_entry("int128", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint136_t sea_nd_u136(void);
sol_raw_uint136_t nd_uint136_t(const char* _msg)
{
	sol_raw_int136_t v = sea_nd_u136();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint136", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int136_t sea_nd_i136(void);
sol_raw_int136_t nd_int136_t(const char* _msg)
{
	sol_raw_int136_t v = sea_nd_i136();
	#ifdef MC_LOG_ALL
	on_entry("int136", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint144_t sea_nd_u144(void);
sol_raw_uint144_t nd_uint144_t(const char* _msg)
{
	sol_raw_int144_t v = sea_nd_u144();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint144", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int144_t sea_nd_i144(void);
sol_raw_int144_t nd_int144_t(const char* _msg)
{
	sol_raw_int144_t v = sea_nd_i144();
	#ifdef MC_LOG_ALL
	on_entry("int144", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint152_t sea_nd_u152(void);
sol_raw_uint152_t nd_uint152_t(const char* _msg)
{
	sol_raw_int152_t v = sea_nd_u152();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint152", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int152_t sea_nd_i152(void);
sol_raw_int152_t nd_int152_t(const char* _msg)
{
	sol_raw_int152_t v = sea_nd_i152();
	#ifdef MC_LOG_ALL
	on_entry("int152", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint160_t sea_nd_u160(void);
sol_raw_uint160_t nd_uint160_t(const char* _msg)
{
	sol_raw_int160_t v = sea_nd_u160();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint160", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int160_t sea_nd_i160(void);
sol_raw_int160_t nd_int160_t(const char* _msg)
{
	sol_raw_int160_t v = sea_nd_i160();
	#ifdef MC_LOG_ALL
	on_entry("int160", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint168_t sea_nd_u168(void);
sol_raw_uint168_t nd_uint168_t(const char* _msg)
{
	sol_raw_int168_t v = sea_nd_u168();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint168", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int168_t sea_nd_i168(void);
sol_raw_int168_t nd_int168_t(const char* _msg)
{
	sol_raw_int168_t v = sea_nd_i168();
	#ifdef MC_LOG_ALL
	on_entry("int168", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint176_t sea_nd_u176(void);
sol_raw_uint176_t nd_uint176_t(const char* _msg)
{
	sol_raw_int176_t v = sea_nd_u176();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint176", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int176_t sea_nd_i176(void);
sol_raw_int176_t nd_int176_t(const char* _msg)
{
	sol_raw_int176_t v = sea_nd_i176();
	#ifdef MC_LOG_ALL
	on_entry("int176", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint184_t sea_nd_u184(void);
sol_raw_uint184_t nd_uint184_t(const char* _msg)
{
	sol_raw_int184_t v = sea_nd_u184();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint184", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int184_t sea_nd_i184(void);
sol_raw_int184_t nd_int184_t(const char* _msg)
{
	sol_raw_int184_t v = sea_nd_i184();
	#ifdef MC_LOG_ALL
	on_entry("int184", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint192_t sea_nd_u192(void);
sol_raw_uint192_t nd_uint192_t(const char* _msg)
{
	sol_raw_int192_t v = sea_nd_u192();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint192", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int192_t sea_nd_i192(void);
sol_raw_int192_t nd_int192_t(const char* _msg)
{
	sol_raw_int192_t v = sea_nd_i192();
	#ifdef MC_LOG_ALL
	on_entry("int192", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint200_t sea_nd_u200(void);
sol_raw_uint200_t nd_uint200_t(const char* _msg)
{
	sol_raw_int200_t v = sea_nd_u200();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint200", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int200_t sea_nd_i200(void);
sol_raw_int200_t nd_int200_t(const char* _msg)
{
	sol_raw_int200_t v = sea_nd_i200();
	#ifdef MC_LOG_ALL
	on_entry("int200", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint208_t sea_nd_u208(void);
sol_raw_uint208_t nd_uint208_t(const char* _msg)
{
	sol_raw_int208_t v = sea_nd_u208();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint208", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int208_t sea_nd_i208(void);
sol_raw_int208_t nd_int208_t(const char* _msg)
{
	sol_raw_int208_t v = sea_nd_i208();
	#ifdef MC_LOG_ALL
	on_entry("int208", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint216_t sea_nd_u216(void);
sol_raw_uint216_t nd_uint216_t(const char* _msg)
{
	sol_raw_int216_t v = sea_nd_u216();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint216", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int216_t sea_nd_i216(void);
sol_raw_int216_t nd_int216_t(const char* _msg)
{
	sol_raw_int216_t v = sea_nd_i216();
	#ifdef MC_LOG_ALL
	on_entry("int216", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint224_t sea_nd_u224(void);
sol_raw_uint224_t nd_uint224_t(const char* _msg)
{
	sol_raw_int224_t v = sea_nd_u224();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint224", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int224_t sea_nd_i224(void);
sol_raw_int224_t nd_int224_t(const char* _msg)
{
	sol_raw_int224_t v = sea_nd_i224();
	#ifdef MC_LOG_ALL
	on_entry("int224", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint232_t sea_nd_u232(void);
sol_raw_uint232_t nd_uint232_t(const char* _msg)
{
	sol_raw_int232_t v = sea_nd_u232();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("int232", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int232_t sea_nd_i232(void);
sol_raw_int232_t nd_int232_t(const char* _msg)
{
	sol_raw_int232_t v = sea_nd_i232();
	#ifdef MC_LOG_ALL
	on_entry("int232", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint240_t sea_nd_u240(void);
sol_raw_uint240_t nd_uint240_t(const char* _msg)
{
	sol_raw_int240_t v = sea_nd_u240();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint240", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int240_t sea_nd_i240(void);
sol_raw_int240_t nd_int240_t(const char* _msg)
{
	sol_raw_int240_t v = sea_nd_i240();
	#ifdef MC_LOG_ALL
	on_entry("int240", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint248_t sea_nd_u248(void);
sol_raw_uint248_t nd_uint248_t(const char* _msg)
{
	sol_raw_int248_t v = sea_nd_u248();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint248", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int248_t sea_nd_i248(void);
sol_raw_int248_t nd_int248_t(const char* _msg)
{
	sol_raw_int248_t v = sea_nd_i248();
	#ifdef MC_LOG_ALL
	on_entry("int248", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_uint256_t sea_nd_u256(void);
sol_raw_uint256_t nd_uint256_t(const char* _msg)
{
	sol_raw_int256_t v = sea_nd_u256();
	assume(v >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint256", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

extern sol_raw_int256_t sea_nd_i256(void);
sol_raw_int256_t nd_int256_t(const char* _msg)
{
 	sol_raw_int256_t v = sea_nd_i256();
	#ifdef MC_LOG_ALL
	on_entry("int256", _msg);
	printf("%ld\n", v);
	#endif
	return v;
}

// -------------------------------------------------------------------------- //
