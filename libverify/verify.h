/**
 * Defines an interface to libverify, with reconfigurable data types. These
 * types are controlled at build time using preprocessor definitions.
 */

#pragma once

// TODO: remove this once the code really uses boost...
#include <stdint.h>

#ifdef MC_USE_BOOST_MP
#include <boost/multiprecision/cpp_int.hpp>
#define SOL_INTEGER_UINT8 uint8_t
#define SOL_INTEGER_INT8 int8_t
#define SOL_INTEGER_UINT16 uint16_t
#define SOL_INTEGER_INT16 int16_t
#define SOL_INTEGER_UINT24 uint32_t
#define SOL_INTEGER_INT24 int32_t
#define SOL_INTEGER_UINT32 uint32_t
#define SOL_INTEGER_INT32 int32_t
#define SOL_INTEGER_UINT40 uint64_t
#define SOL_INTEGER_INT40 int64_t
#define SOL_INTEGER_UINT48 uint64_t
#define SOL_INTEGER_INT48 int64_t
#define SOL_INTEGER_UINT56 uint64_t
#define SOL_INTEGER_INT56 int64_t
#define SOL_INTEGER_UINT64 uint64_t
#define SOL_INTEGER_INT64 int64_t
#define SOL_INTEGER_UINT72 __uint128_t
#define SOL_INTEGER_INT72 __int128_t
#define SOL_INTEGER_UINT80 __uint128_t
#define SOL_INTEGER_INT80 __int128_t
#define SOL_INTEGER_UINT88 __uint128_t
#define SOL_INTEGER_INT88 __int128_t
#define SOL_INTEGER_UINT96 __uint128_t
#define SOL_INTEGER_INT96 __int128_t
#define SOL_INTEGER_UINT104 __uint128_t
#define SOL_INTEGER_INT104 __int128_t
#define SOL_INTEGER_UINT112 __uint128_t
#define SOL_INTEGER_INT112 __int128_t
#define SOL_INTEGER_UINT120 __uint128_t
#define SOL_INTEGER_INT120 __int128_t
#define SOL_INTEGER_UINT128 __uint128_t
#define SOL_INTEGER_INT128 __int128_t
#define SOL_INTEGER_UINT136 __uint128_t
#define SOL_INTEGER_INT136 __int128_t
#define SOL_INTEGER_UINT144 __uint128_t
#define SOL_INTEGER_INT144 __int128_t
#define SOL_INTEGER_UINT152 __uint128_t
#define SOL_INTEGER_INT152 __int128_t
#define SOL_INTEGER_UINT160 __uint128_t
#define SOL_INTEGER_INT160 __int128_t
#define SOL_INTEGER_UINT168 __uint128_t
#define SOL_INTEGER_INT168 __int128_t
#define SOL_INTEGER_UINT176 __uint128_t
#define SOL_INTEGER_INT176 __int128_t
#define SOL_INTEGER_UINT184 __uint128_t
#define SOL_INTEGER_INT184 __int128_t
#define SOL_INTEGER_UINT192 __uint128_t
#define SOL_INTEGER_INT192 __int128_t
#define SOL_INTEGER_UINT200 __uint128_t
#define SOL_INTEGER_INT200 __int128_t
#define SOL_INTEGER_UINT208 __uint128_t
#define SOL_INTEGER_INT208 __int128_t
#define SOL_INTEGER_UINT216 __uint128_t
#define SOL_INTEGER_INT216 __int128_t
#define SOL_INTEGER_UINT224 __uint128_t
#define SOL_INTEGER_INT224 __int128_t
#define SOL_INTEGER_UINT232 __uint128_t
#define SOL_INTEGER_INT232 __int128_t
#define SOL_INTEGER_UINT240 __uint128_t
#define SOL_INTEGER_INT240 __int128_t
#define SOL_INTEGER_UINT248 __uint128_t
#define SOL_INTEGER_INT248 __int128_t
#define SOL_INTEGER_UINT256 __uint128_t
#define SOL_INTEGER_INT256 __int128_t
#elif defined MC_USE_STDINT
#include <stdint.h>
#define SOL_INTEGER_UINT8 uint8_t
#define SOL_INTEGER_INT8 int8_t
#define SOL_INTEGER_UINT16 uint16_t
#define SOL_INTEGER_INT16 int16_t
#define SOL_INTEGER_UINT24 uint32_t
#define SOL_INTEGER_INT24 int32_t
#define SOL_INTEGER_UINT32 uint32_t
#define SOL_INTEGER_INT32 int32_t
#define SOL_INTEGER_UINT40 uint64_t
#define SOL_INTEGER_INT40 int64_t
#define SOL_INTEGER_UINT48 uint64_t
#define SOL_INTEGER_INT48 int64_t
#define SOL_INTEGER_UINT56 uint64_t
#define SOL_INTEGER_INT56 int64_t
#define SOL_INTEGER_UINT64 uint64_t
#define SOL_INTEGER_INT64 int64_t
#define SOL_INTEGER_UINT72 __uint128_t
#define SOL_INTEGER_INT72 __int128_t
#define SOL_INTEGER_UINT80 __uint128_t
#define SOL_INTEGER_INT80 __int128_t
#define SOL_INTEGER_UINT88 __uint128_t
#define SOL_INTEGER_INT88 __int128_t
#define SOL_INTEGER_UINT96 __uint128_t
#define SOL_INTEGER_INT96 __int128_t
#define SOL_INTEGER_UINT104 __uint128_t
#define SOL_INTEGER_INT104 __int128_t
#define SOL_INTEGER_UINT112 __uint128_t
#define SOL_INTEGER_INT112 __int128_t
#define SOL_INTEGER_UINT120 __uint128_t
#define SOL_INTEGER_INT120 __int128_t
#define SOL_INTEGER_UINT128 __uint128_t
#define SOL_INTEGER_INT128 __int128_t
#define SOL_INTEGER_UINT136 __uint128_t
#define SOL_INTEGER_INT136 __int128_t
#define SOL_INTEGER_UINT144 __uint128_t
#define SOL_INTEGER_INT144 __int128_t
#define SOL_INTEGER_UINT152 __uint128_t
#define SOL_INTEGER_INT152 __int128_t
#define SOL_INTEGER_UINT160 __uint128_t
#define SOL_INTEGER_INT160 __int128_t
#define SOL_INTEGER_UINT168 __uint128_t
#define SOL_INTEGER_INT168 __int128_t
#define SOL_INTEGER_UINT176 __uint128_t
#define SOL_INTEGER_INT176 __int128_t
#define SOL_INTEGER_UINT184 __uint128_t
#define SOL_INTEGER_INT184 __int128_t
#define SOL_INTEGER_UINT192 __uint128_t
#define SOL_INTEGER_INT192 __int128_t
#define SOL_INTEGER_UINT200 __uint128_t
#define SOL_INTEGER_INT200 __int128_t
#define SOL_INTEGER_UINT208 __uint128_t
#define SOL_INTEGER_INT208 __int128_t
#define SOL_INTEGER_UINT216 __uint128_t
#define SOL_INTEGER_INT216 __int128_t
#define SOL_INTEGER_UINT224 __uint128_t
#define SOL_INTEGER_INT224 __int128_t
#define SOL_INTEGER_UINT232 __uint128_t
#define SOL_INTEGER_INT232 __int128_t
#define SOL_INTEGER_UINT240 __uint128_t
#define SOL_INTEGER_INT240 __int128_t
#define SOL_INTEGER_UINT248 __uint128_t
#define SOL_INTEGER_INT248 __int128_t
#define SOL_INTEGER_UINT256 __uint128_t
#define SOL_INTEGER_INT256 __int128_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

void sol_require(SOL_INTEGER_UINT8 _cond, const char* _msg);
void sol_assert(SOL_INTEGER_UINT8 cond, const char* _msg);

SOL_INTEGER_INT8 nd_int8_t(const char* _msg);
SOL_INTEGER_UINT8 nd_uint8_t(const char* _msg);
SOL_INTEGER_INT16 nd_int16_t(const char* _msg);
SOL_INTEGER_UINT16 nd_uint16_t(const char* _msg);
SOL_INTEGER_INT24 nd_int24_t(const char* _msg);
SOL_INTEGER_UINT24 nd_uint24_t(const char* _msg);
SOL_INTEGER_INT32 nd_int32_t(const char* _msg);
SOL_INTEGER_UINT32 nd_uint32_t(const char* _msg);
SOL_INTEGER_INT40 nd_int40_t(const char* _msg);
SOL_INTEGER_UINT40 nd_uint40_t(const char* _msg);
SOL_INTEGER_INT48 nd_int48_t(const char* _msg);
SOL_INTEGER_UINT48 nd_uint48_t(const char* _msg);
SOL_INTEGER_INT56 nd_int56_t(const char* _msg);
SOL_INTEGER_UINT56 nd_uint56_t(const char* _msg);
SOL_INTEGER_INT64 nd_int64_t(const char* _msg);
SOL_INTEGER_UINT64 nd_uint64_t(const char* _msg);
SOL_INTEGER_INT72 nd_int72_t(const char* _msg);
SOL_INTEGER_UINT72 nd_uint72_t(const char* _msg);
SOL_INTEGER_INT80 nd_int80_t(const char* _msg);
SOL_INTEGER_UINT80 nd_uint80_t(const char* _msg);
SOL_INTEGER_INT88 nd_int88_t(const char* _msg);
SOL_INTEGER_UINT88 nd_uint88_t(const char* _msg);
SOL_INTEGER_INT96 nd_int96_t(const char* _msg);
SOL_INTEGER_UINT96 nd_uint96_t(const char* _msg);
SOL_INTEGER_INT104 nd_int104_t(const char* _msg);
SOL_INTEGER_UINT104 nd_uint104_t(const char* _msg);
SOL_INTEGER_INT112 nd_int112_t(const char* _msg);
SOL_INTEGER_UINT112 nd_uint112_t(const char* _msg);
SOL_INTEGER_INT120 nd_int120_t(const char* _msg);
SOL_INTEGER_UINT120 nd_uint120_t(const char* _msg);
SOL_INTEGER_INT128 nd_int128_t(const char* _msg);
SOL_INTEGER_UINT128 nd_uint128_t(const char* _msg);
SOL_INTEGER_INT136 nd_int136_t(const char* _msg);
SOL_INTEGER_UINT136 nd_uint136_t(const char* _msg);
SOL_INTEGER_INT144 nd_int144_t(const char* _msg);
SOL_INTEGER_UINT144 nd_uint144_t(const char* _msg);
SOL_INTEGER_INT152 nd_int152_t(const char* _msg);
SOL_INTEGER_UINT152 nd_uint152_t(const char* _msg);
SOL_INTEGER_INT160 nd_int160_t(const char* _msg);
SOL_INTEGER_UINT160 nd_uint160_t(const char* _msg);
SOL_INTEGER_INT168 nd_int168_t(const char* _msg);
SOL_INTEGER_UINT168 nd_uint168_t(const char* _msg);
SOL_INTEGER_INT176 nd_int176_t(const char* _msg);
SOL_INTEGER_UINT176 nd_uint176_t(const char* _msg);
SOL_INTEGER_INT184 nd_int184_t(const char* _msg);
SOL_INTEGER_UINT184 nd_uint184_t(const char* _msg);
SOL_INTEGER_INT192 nd_int192_t(const char* _msg);
SOL_INTEGER_UINT192 nd_uint192_t(const char* _msg);
SOL_INTEGER_INT200 nd_int200_t(const char* _msg);
SOL_INTEGER_UINT200 nd_uint200_t(const char* _msg);
SOL_INTEGER_INT208 nd_int208_t(const char* _msg);
SOL_INTEGER_UINT208 nd_uint208_t(const char* _msg);
SOL_INTEGER_INT216 nd_int216_t(const char* _msg);
SOL_INTEGER_UINT216 nd_uint216_t(const char* _msg);
SOL_INTEGER_INT224 nd_int224_t(const char* _msg);
SOL_INTEGER_UINT224 nd_uint224_t(const char* _msg);
SOL_INTEGER_INT232 nd_int232_t(const char* _msg);
SOL_INTEGER_UINT232 nd_uint232_t(const char* _msg);
SOL_INTEGER_INT240 nd_int240_t(const char* _msg);
SOL_INTEGER_UINT240 nd_uint240_t(const char* _msg);
SOL_INTEGER_INT248 nd_int248_t(const char* _msg);
SOL_INTEGER_UINT248 nd_uint248_t(const char* _msg);
SOL_INTEGER_INT256 nd_int256_t(const char* _msg);
SOL_INTEGER_UINT256 nd_uint256_t(const char* _msg);

#ifdef __cplusplus
}
#endif
