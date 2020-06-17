/**
 * Defines assert, require and nd implementations for symbolic execution.
 * @date 2019
 */

#include "verify.h"

#include "klee/klee.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

// -------------------------------------------------------------------------- //

sol_raw_uint8_t sol_crypto(void)
{
    return nd_byte("Select crypto value");
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

void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    klee_assert(_cond);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
    klee_assume(_cond);
}

// -------------------------------------------------------------------------- //

void ll_assume(sol_raw_uint8_t _cond)
{
	klee_assume(_cond);
}

// -------------------------------------------------------------------------- //

void sol_emit(const char* _msg) {}

// -------------------------------------------------------------------------- //

uint8_t nd_byte(const char* _msg)
{
    uint8_t res;
    klee_make_symbolic(&res, sizeof(res), _msg);
    return res;
}

uint8_t nd_range(uint8_t l, uint8_t u, const char* _msg)
{
    uint8_t res;
    klee_make_symbolic(&res, sizeof(res), _msg);
	ll_assume(res >= l);
	ll_assume(res < u);
    return res;
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

void smartace_log(const char* _msg) {}

// -------------------------------------------------------------------------- //

sol_raw_int8_t nd_int8_t(const char* _msg){
    sol_raw_int8_t res;
    klee_make_symbolic(&res, sizeof(res), _msg);
    return res;
}
sol_raw_uint8_t nd_uint8_t(const char* _msg){
	sol_raw_uint8_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int16_t nd_int16_t(const char* _msg){
	sol_raw_int16_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint16_t nd_uint16_t(const char* _msg){
	sol_raw_uint16_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int24_t nd_int24_t(const char* _msg){
	sol_raw_int24_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint24_t nd_uint24_t(const char* _msg){
	sol_raw_uint24_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int32_t nd_int32_t(const char* _msg){
	sol_raw_int32_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint32_t nd_uint32_t(const char* _msg){
	sol_raw_uint32_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int40_t nd_int40_t(const char* _msg){
	sol_raw_int40_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint40_t nd_uint40_t(const char* _msg){
	sol_raw_uint40_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int48_t nd_int48_t(const char* _msg){
	sol_raw_int48_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint48_t nd_uint48_t(const char* _msg){
	sol_raw_uint48_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int56_t nd_int56_t(const char* _msg){
	sol_raw_int56_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint56_t nd_uint56_t(const char* _msg){
	sol_raw_uint56_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int64_t nd_int64_t(const char* _msg){
	sol_raw_int64_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint64_t nd_uint64_t(const char* _msg){
	sol_raw_uint64_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int72_t nd_int72_t(const char* _msg){
	sol_raw_int72_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint72_t nd_uint72_t(const char* _msg){
	sol_raw_uint72_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int80_t nd_int80_t(const char* _msg){
	sol_raw_int80_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint80_t nd_uint80_t(const char* _msg){
	sol_raw_uint80_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int88_t nd_int88_t(const char* _msg){
	sol_raw_int88_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint88_t nd_uint88_t(const char* _msg){
	sol_raw_uint88_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int96_t nd_int96_t(const char* _msg){
	sol_raw_int96_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint96_t nd_uint96_t(const char* _msg){
	sol_raw_uint96_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int104_t nd_int104_t(const char* _msg){
	sol_raw_int104_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint104_t nd_uint104_t(const char* _msg){
	sol_raw_uint104_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int112_t nd_int112_t(const char* _msg){
	sol_raw_int112_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint112_t nd_uint112_t(const char* _msg){
	sol_raw_uint112_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int120_t nd_int120_t(const char* _msg){
	sol_raw_int120_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint120_t nd_uint120_t(const char* _msg){
	sol_raw_uint120_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int128_t nd_int128_t(const char* _msg){
	sol_raw_int128_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint128_t nd_uint128_t(const char* _msg){
	sol_raw_uint128_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int136_t nd_int136_t(const char* _msg){
	sol_raw_int136_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint136_t nd_uint136_t(const char* _msg){
	sol_raw_uint136_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int144_t nd_int144_t(const char* _msg){
	sol_raw_int144_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint144_t nd_uint144_t(const char* _msg){
	sol_raw_uint144_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int152_t nd_int152_t(const char* _msg){
	sol_raw_int152_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint152_t nd_uint152_t(const char* _msg){
	sol_raw_uint152_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int160_t nd_int160_t(const char* _msg){
	sol_raw_int160_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint160_t nd_uint160_t(const char* _msg){
	sol_raw_uint160_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int168_t nd_int168_t(const char* _msg){
	sol_raw_int168_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint168_t nd_uint168_t(const char* _msg){
	sol_raw_uint168_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int176_t nd_int176_t(const char* _msg){
	sol_raw_int176_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint176_t nd_uint176_t(const char* _msg){
	sol_raw_uint176_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int184_t nd_int184_t(const char* _msg){
	sol_raw_int184_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint184_t nd_uint184_t(const char* _msg){
	sol_raw_uint184_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int192_t nd_int192_t(const char* _msg){
	sol_raw_int192_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint192_t nd_uint192_t(const char* _msg){
	sol_raw_uint192_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int200_t nd_int200_t(const char* _msg){
	sol_raw_int200_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint200_t nd_uint200_t(const char* _msg){
	sol_raw_uint200_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int208_t nd_int208_t(const char* _msg){
	sol_raw_int208_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint208_t nd_uint208_t(const char* _msg){
	sol_raw_uint208_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int216_t nd_int216_t(const char* _msg){
	sol_raw_int216_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint216_t nd_uint216_t(const char* _msg){
	sol_raw_uint216_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int224_t nd_int224_t(const char* _msg){
	sol_raw_int224_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint224_t nd_uint224_t(const char* _msg){
	sol_raw_uint224_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int232_t nd_int232_t(const char* _msg){
	sol_raw_int232_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint232_t nd_uint232_t(const char* _msg){
	sol_raw_uint232_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int240_t nd_int240_t(const char* _msg){
	sol_raw_int240_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint240_t nd_uint240_t(const char* _msg){
	sol_raw_uint240_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int248_t nd_int248_t(const char* _msg){
	sol_raw_int248_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint248_t nd_uint248_t(const char* _msg){
	sol_raw_uint248_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_int256_t nd_int256_t(const char* _msg){
	sol_raw_int256_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}
sol_raw_uint256_t nd_uint256_t(const char* _msg){
	sol_raw_uint256_t res;
	klee_make_symbolic(&res, sizeof(res), _msg);
	return res;
}

// -------------------------------------------------------------------------- //
