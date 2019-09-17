/**
 * Defines an interface to libverify, with reconfigurable data types. These
 * types are controlled at build time using preprocessor definitions.
 */

#pragma once

// TODO: remove this once the code really uses boost...
#include <stdint.h>

#ifdef MC_USE_BOOST_MP
#include <boost/multiprecision/cpp_int.hpp>
typedef int8_t sol_raw_int8_t;
typedef uint8_t sol_raw_uint8_t;
typedef int16_t sol_raw_int16_t;
typedef uint16_t sol_raw_uint16_t;
typedef int32_t sol_raw_int24_t;
typedef uint32_t sol_raw_uint24_t;
typedef int32_t sol_raw_int32_t;
typedef uint32_t sol_raw_uint32_t;
typedef int64_t sol_raw_int40_t;
typedef uint64_t sol_raw_uint40_t;
typedef int64_t sol_raw_int48_t;
typedef uint64_t sol_raw_uint48_t;
typedef int64_t sol_raw_int56_t;
typedef uint64_t sol_raw_uint56_t;
typedef int64_t sol_raw_int64_t;
typedef uint64_t sol_raw_uint64_t;
typedef __int128_t sol_raw_int72_t;
typedef __uint128_t sol_raw_uint72_t;
typedef __int128_t sol_raw_int80_t;
typedef __uint128_t sol_raw_uint80_t;
typedef __int128_t sol_raw_int88_t;
typedef __uint128_t sol_raw_uint88_t;
typedef __int128_t sol_raw_int96_t;
typedef __uint128_t sol_raw_uint96_t;
typedef __int128_t sol_raw_int104_t;
typedef __uint128_t sol_raw_uint104_t;
typedef __int128_t sol_raw_int112_t;
typedef __uint128_t sol_raw_uint112_t;
typedef __int128_t sol_raw_int120_t;
typedef __uint128_t sol_raw_uint120_t;
typedef __int128_t sol_raw_int128_t;
typedef __uint128_t sol_raw_uint128_t;
typedef __int128_t sol_raw_int136_t;
typedef __uint128_t sol_raw_uint136_t;
typedef __int128_t sol_raw_int144_t;
typedef __uint128_t sol_raw_uint144_t;
typedef __int128_t sol_raw_int152_t;
typedef __uint128_t sol_raw_uint152_t;
typedef __int128_t sol_raw_int160_t;
typedef __uint128_t sol_raw_uint160_t;
typedef __int128_t sol_raw_int168_t;
typedef __uint128_t sol_raw_uint168_t;
typedef __int128_t sol_raw_int176_t;
typedef __uint128_t sol_raw_uint176_t;
typedef __int128_t sol_raw_int184_t;
typedef __uint128_t sol_raw_uint184_t;
typedef __int128_t sol_raw_int192_t;
typedef __uint128_t sol_raw_uint192_t;
typedef __int128_t sol_raw_int200_t;
typedef __uint128_t sol_raw_uint200_t;
typedef __int128_t sol_raw_int208_t;
typedef __uint128_t sol_raw_uint208_t;
typedef __int128_t sol_raw_int216_t;
typedef __uint128_t sol_raw_uint216_t;
typedef __int128_t sol_raw_int224_t;
typedef __uint128_t sol_raw_uint224_t;
typedef __int128_t sol_raw_int232_t;
typedef __uint128_t sol_raw_uint232_t;
typedef __int128_t sol_raw_int240_t;
typedef __uint128_t sol_raw_uint240_t;
typedef __int128_t sol_raw_int248_t;
typedef __uint128_t sol_raw_uint248_t;
typedef __int128_t sol_raw_int256_t;
typedef __uint128_t sol_raw_uint256_t;
#elif defined MC_USE_STDINT
typedef int8_t sol_raw_int8_t;
typedef uint8_t sol_raw_uint8_t;
typedef int16_t sol_raw_int16_t;
typedef uint16_t sol_raw_uint16_t;
typedef int32_t sol_raw_int24_t;
typedef uint32_t sol_raw_uint24_t;
typedef int32_t sol_raw_int32_t;
typedef uint32_t sol_raw_uint32_t;
typedef int64_t sol_raw_int40_t;
typedef uint64_t sol_raw_uint40_t;
typedef int64_t sol_raw_int48_t;
typedef uint64_t sol_raw_uint48_t;
typedef int64_t sol_raw_int56_t;
typedef uint64_t sol_raw_uint56_t;
typedef int64_t sol_raw_int64_t;
typedef uint64_t sol_raw_uint64_t;
typedef __int128_t sol_raw_int72_t;
typedef __uint128_t sol_raw_uint72_t;
typedef __int128_t sol_raw_int80_t;
typedef __uint128_t sol_raw_uint80_t;
typedef __int128_t sol_raw_int88_t;
typedef __uint128_t sol_raw_uint88_t;
typedef __int128_t sol_raw_int96_t;
typedef __uint128_t sol_raw_uint96_t;
typedef __int128_t sol_raw_int104_t;
typedef __uint128_t sol_raw_uint104_t;
typedef __int128_t sol_raw_int112_t;
typedef __uint128_t sol_raw_uint112_t;
typedef __int128_t sol_raw_int120_t;
typedef __uint128_t sol_raw_uint120_t;
typedef __int128_t sol_raw_int128_t;
typedef __uint128_t sol_raw_uint128_t;
typedef __int128_t sol_raw_int136_t;
typedef __uint128_t sol_raw_uint136_t;
typedef __int128_t sol_raw_int144_t;
typedef __uint128_t sol_raw_uint144_t;
typedef __int128_t sol_raw_int152_t;
typedef __uint128_t sol_raw_uint152_t;
typedef __int128_t sol_raw_int160_t;
typedef __uint128_t sol_raw_uint160_t;
typedef __int128_t sol_raw_int168_t;
typedef __uint128_t sol_raw_uint168_t;
typedef __int128_t sol_raw_int176_t;
typedef __uint128_t sol_raw_uint176_t;
typedef __int128_t sol_raw_int184_t;
typedef __uint128_t sol_raw_uint184_t;
typedef __int128_t sol_raw_int192_t;
typedef __uint128_t sol_raw_uint192_t;
typedef __int128_t sol_raw_int200_t;
typedef __uint128_t sol_raw_uint200_t;
typedef __int128_t sol_raw_int208_t;
typedef __uint128_t sol_raw_uint208_t;
typedef __int128_t sol_raw_int216_t;
typedef __uint128_t sol_raw_uint216_t;
typedef __int128_t sol_raw_int224_t;
typedef __uint128_t sol_raw_uint224_t;
typedef __int128_t sol_raw_int232_t;
typedef __uint128_t sol_raw_uint232_t;
typedef __int128_t sol_raw_int240_t;
typedef __uint128_t sol_raw_uint240_t;
typedef __int128_t sol_raw_int248_t;
typedef __uint128_t sol_raw_uint248_t;
typedef __int128_t sol_raw_int256_t;
typedef __uint128_t sol_raw_uint256_t;
#else
#error An integer model is requried.
#endif

#ifdef __cplusplus
extern "C" {
#endif

void sol_require(sol_raw_uint8_t _cond, const char* _msg);
void sol_assert(sol_raw_uint8_t cond, const char* _msg);

sol_raw_int8_t nd_int8_t(const char* _msg);
sol_raw_uint8_t nd_uint8_t(const char* _msg);
sol_raw_int16_t nd_int16_t(const char* _msg);
sol_raw_uint16_t nd_uint16_t(const char* _msg);
sol_raw_int24_t nd_int24_t(const char* _msg);
sol_raw_uint24_t nd_uint24_t(const char* _msg);
sol_raw_int32_t nd_int32_t(const char* _msg);
sol_raw_uint32_t nd_uint32_t(const char* _msg);
sol_raw_int40_t nd_int40_t(const char* _msg);
sol_raw_uint40_t nd_uint40_t(const char* _msg);
sol_raw_int48_t nd_int48_t(const char* _msg);
sol_raw_uint48_t nd_uint48_t(const char* _msg);
sol_raw_int56_t nd_int56_t(const char* _msg);
sol_raw_uint56_t nd_uint56_t(const char* _msg);
sol_raw_int64_t nd_int64_t(const char* _msg);
sol_raw_uint64_t nd_uint64_t(const char* _msg);
sol_raw_int72_t nd_int72_t(const char* _msg);
sol_raw_uint72_t nd_uint72_t(const char* _msg);
sol_raw_int80_t nd_int80_t(const char* _msg);
sol_raw_uint80_t nd_uint80_t(const char* _msg);
sol_raw_int88_t nd_int88_t(const char* _msg);
sol_raw_uint88_t nd_uint88_t(const char* _msg);
sol_raw_int96_t nd_int96_t(const char* _msg);
sol_raw_uint96_t nd_uint96_t(const char* _msg);
sol_raw_int104_t nd_int104_t(const char* _msg);
sol_raw_uint104_t nd_uint104_t(const char* _msg);
sol_raw_int112_t nd_int112_t(const char* _msg);
sol_raw_uint112_t nd_uint112_t(const char* _msg);
sol_raw_int120_t nd_int120_t(const char* _msg);
sol_raw_uint120_t nd_uint120_t(const char* _msg);
sol_raw_int128_t nd_int128_t(const char* _msg);
sol_raw_uint128_t nd_uint128_t(const char* _msg);
sol_raw_int136_t nd_int136_t(const char* _msg);
sol_raw_uint136_t nd_uint136_t(const char* _msg);
sol_raw_int144_t nd_int144_t(const char* _msg);
sol_raw_uint144_t nd_uint144_t(const char* _msg);
sol_raw_int152_t nd_int152_t(const char* _msg);
sol_raw_uint152_t nd_uint152_t(const char* _msg);
sol_raw_int160_t nd_int160_t(const char* _msg);
sol_raw_uint160_t nd_uint160_t(const char* _msg);
sol_raw_int168_t nd_int168_t(const char* _msg);
sol_raw_uint168_t nd_uint168_t(const char* _msg);
sol_raw_int176_t nd_int176_t(const char* _msg);
sol_raw_uint176_t nd_uint176_t(const char* _msg);
sol_raw_int184_t nd_int184_t(const char* _msg);
sol_raw_uint184_t nd_uint184_t(const char* _msg);
sol_raw_int192_t nd_int192_t(const char* _msg);
sol_raw_uint192_t nd_uint192_t(const char* _msg);
sol_raw_int200_t nd_int200_t(const char* _msg);
sol_raw_uint200_t nd_uint200_t(const char* _msg);
sol_raw_int208_t nd_int208_t(const char* _msg);
sol_raw_uint208_t nd_uint208_t(const char* _msg);
sol_raw_int216_t nd_int216_t(const char* _msg);
sol_raw_uint216_t nd_uint216_t(const char* _msg);
sol_raw_int224_t nd_int224_t(const char* _msg);
sol_raw_uint224_t nd_uint224_t(const char* _msg);
sol_raw_int232_t nd_int232_t(const char* _msg);
sol_raw_uint232_t nd_uint232_t(const char* _msg);
sol_raw_int240_t nd_int240_t(const char* _msg);
sol_raw_uint240_t nd_uint240_t(const char* _msg);
sol_raw_int248_t nd_int248_t(const char* _msg);
sol_raw_uint248_t nd_uint248_t(const char* _msg);
sol_raw_int256_t nd_int256_t(const char* _msg);
sol_raw_uint256_t nd_uint256_t(const char* _msg);

#ifdef __cplusplus
}
#endif
