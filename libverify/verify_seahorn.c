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

extern sol_raw_uint8_t nd_crypto(void);
sol_raw_uint8_t sol_crypto(void)
{
    return nd_byte(nd_crypto(), "Select crypto value");
}

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

uint8_t sol_is_using_reps(void)
{
    return 1;
}

// -------------------------------------------------------------------------- //

#ifdef MC_LOG_ALL
void log_assertion(const char* _type, sol_raw_uint8_t _cond, const char* _msg)
{
	if (!_cond)
	{
		if (_msg)
		{
			printf("%s: %s\n", _type, _msg);
		}
		else
		{
			printf("%s\n", _type);
		}
	}
}
#endif

void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    (void) _msg;
	#ifdef MC_LOG_ALL
    log_assertion("assert", _cond, _msg);
	#endif
    sassert(_cond);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
    (void) _msg;
	#ifdef MC_LOG_ALL
    log_assertion("require", _cond, _msg);
	#endif
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

uint8_t nd_byte(int8_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint8", _msg);
    printf("%hhu\n", _sea_hint);
	#endif
    return _sea_hint;
}

uint8_t nd_range(int8_t _sea_hint, uint8_t l, uint8_t u, const char* _msg)
{
	ll_assume(_sea_hint >= l);
	ll_assume(_sea_hint < u);
	#ifdef MC_LOG_ALL
	on_entry("uint8", _msg);
    printf("%hhu\n", _sea_hint);
	#endif
	return _sea_hint;
}

// -------------------------------------------------------------------------- //

sol_raw_uint256_t nd_increase(
	sol_raw_int256_t _sea_hint,
	sol_raw_uint256_t _curr,
	uint8_t _strict,
	const char* _msg
)
{
	if (_strict) ll_assume(_sea_hint > _curr);
	else ll_assume(_sea_hint >= _curr);
	#ifdef MC_LOG_ALL
	on_entry("uint256", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

// -------------------------------------------------------------------------- //

void smartace_log(const char* _msg)
{
	#ifdef MC_LOG_ALL
	printf("%s\n", _msg);
	#endif
}

// -------------------------------------------------------------------------- //

sol_raw_uint8_t nd_uint8_t(sol_raw_int8_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint8", _msg);
	printf("%hhu\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int8_t nd_int8_t(sol_raw_int8_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int8", _msg);
	printf("%hhu\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint16_t nd_uint16_t(sol_raw_int16_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint16", _msg);
	printf("%hu\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int16_t nd_int16_t(sol_raw_int16_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int16", _msg);
	printf("%hd\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint24_t nd_uint24_t(sol_raw_int24_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint24", _msg);
	printf("%u\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int24_t nd_int24_t(sol_raw_int24_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int24", _msg);
	printf("%d\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint32_t nd_uint32_t(sol_raw_int32_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint32", _msg);
	printf("%u\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int32_t nd_int32_t(sol_raw_int32_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int32", _msg);
	printf("%d\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint40_t nd_uint40_t(sol_raw_int40_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint40", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int40_t nd_int40_t(sol_raw_int40_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int40", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint48_t nd_uint48_t(sol_raw_int48_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uin48", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int48_t nd_int48_t(sol_raw_int48_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int48", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint56_t nd_uint56_t(sol_raw_int56_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint56", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int56_t nd_int56_t(sol_raw_int56_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int56", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint64_t nd_uint64_t(sol_raw_int64_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint64", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int64_t nd_int64_t(sol_raw_int64_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int64", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint72_t nd_uint72_t(sol_raw_int72_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint72", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int72_t nd_int72_t(sol_raw_int72_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int72", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint80_t nd_uint80_t(sol_raw_int80_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint80", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int80_t nd_int80_t(sol_raw_int80_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("uint80", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint88_t nd_uint88_t(sol_raw_int88_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint88", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int88_t nd_int88_t(sol_raw_int88_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int88", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint96_t nd_uint96_t(sol_raw_int96_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint96", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int96_t nd_int96_t(sol_raw_int96_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int96", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint104_t nd_uint104_t(sol_raw_int104_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint104", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int104_t nd_int104_t(sol_raw_int104_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int104", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint112_t nd_uint112_t(sol_raw_int112_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint104", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int112_t nd_int112_t(sol_raw_int112_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int112", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint120_t nd_uint120_t(sol_raw_int120_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint112", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int120_t nd_int120_t(sol_raw_int120_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int120", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint128_t nd_uint128_t(sol_raw_int128_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint120", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int128_t nd_int128_t(sol_raw_int128_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int128", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint136_t nd_uint136_t(sol_raw_int136_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint136", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int136_t nd_int136_t(sol_raw_int136_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int136", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint144_t nd_uint144_t(sol_raw_int144_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint144", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int144_t nd_int144_t(sol_raw_int144_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int144", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint152_t nd_uint152_t(sol_raw_int152_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint152", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int152_t nd_int152_t(sol_raw_int152_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int152", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint160_t nd_uint160_t(sol_raw_int160_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint160", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int160_t nd_int160_t(sol_raw_int160_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int160", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint168_t nd_uint168_t(sol_raw_int168_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint168", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int168_t nd_int168_t(sol_raw_int168_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int168", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint176_t nd_uint176_t(sol_raw_int176_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint176", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int176_t nd_int176_t(sol_raw_int176_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int176", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint184_t nd_uint184_t(sol_raw_int184_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint184", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int184_t nd_int184_t(sol_raw_int184_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int184", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint192_t nd_uint192_t(sol_raw_int192_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint192", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int192_t nd_int192_t(sol_raw_int192_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int192", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint200_t nd_uint200_t(sol_raw_int200_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint200", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int200_t nd_int200_t(sol_raw_int200_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int200", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint208_t nd_uint208_t(sol_raw_int208_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint208", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int208_t nd_int208_t(sol_raw_int208_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int208", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint216_t nd_uint216_t(sol_raw_int216_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint216", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int216_t nd_int216_t(sol_raw_int216_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int216", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint224_t nd_uint224_t(sol_raw_int224_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint224", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int224_t nd_int224_t(sol_raw_int224_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int224", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint232_t nd_uint232_t(sol_raw_int232_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("int232", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int232_t nd_int232_t(sol_raw_int232_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int232", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint240_t nd_uint240_t(sol_raw_int240_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint240", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int240_t nd_int240_t(sol_raw_int240_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int240", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint248_t nd_uint248_t(sol_raw_int248_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint248", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int248_t nd_int248_t(sol_raw_int248_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int248", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_uint256_t nd_uint256_t(sol_raw_int256_t _sea_hint, const char* _msg)
{
	ll_assume(_sea_hint >= 0);
	#ifdef MC_LOG_ALL
	on_entry("uint256", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

sol_raw_int256_t nd_int256_t(sol_raw_int256_t _sea_hint, const char* _msg)
{
	#ifdef MC_LOG_ALL
	on_entry("int256", _msg);
	printf("%ld\n", _sea_hint);
	#endif
	return _sea_hint;
}

// -------------------------------------------------------------------------- //
