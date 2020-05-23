/**
 * Defines an interface to libverify, with reconfigurable data types. These
 * types are controlled at build time using preprocessor definitions.
 */

#pragma once

// Macro for ghost variable autoinstrumentation.
#define GHOST_VAR 

// Switches interger implementations based on preprocessor flags.
// TODO: refine this.
#ifdef MC_USE_BOOST_MP
    #ifndef __cplusplus
    #error A C++ compiler is required for Boost.
    #endif
#include <boost/multiprecision/cpp_int.hpp>
#define BOOST_MP_IMPL(BITS, MAG) \
    boost::multiprecision::number<boost::multiprecision::cpp_int_backend< \
        BITS, BITS, MAG, boost::multiprecision::unchecked, void>>
#define BOOST_INT(BITS) \
    BOOST_MP_IMPL(BITS, boost::multiprecision::signed_magnitude)
#define BOOST_UINT(BITS) \
    BOOST_MP_IMPL(BITS, boost::multiprecision::unsigned_magnitude)
typedef BOOST_INT(8) sol_raw_int8_t;
typedef BOOST_UINT(8) sol_raw_uint8_t;
typedef BOOST_INT(16) sol_raw_int16_t;
typedef BOOST_UINT(16) sol_raw_uint16_t;
typedef BOOST_INT(24) sol_raw_int24_t;
typedef BOOST_UINT(24) sol_raw_uint24_t;
typedef BOOST_INT(32) sol_raw_int32_t;
typedef BOOST_UINT(32) sol_raw_uint32_t;
typedef BOOST_INT(40) sol_raw_int40_t;
typedef BOOST_UINT(40) sol_raw_uint40_t;
typedef BOOST_INT(48) sol_raw_int48_t;
typedef BOOST_UINT(48) sol_raw_uint48_t;
typedef BOOST_INT(56) sol_raw_int56_t;
typedef BOOST_UINT(56) sol_raw_uint56_t;
typedef BOOST_INT(64) sol_raw_int64_t;
typedef BOOST_UINT(64) sol_raw_uint64_t;
typedef BOOST_INT(72) sol_raw_int72_t;
typedef BOOST_UINT(72) sol_raw_uint72_t;
typedef BOOST_INT(80) sol_raw_int80_t;
typedef BOOST_UINT(80) sol_raw_uint80_t;
typedef BOOST_INT(88) sol_raw_int88_t;
typedef BOOST_UINT(88) sol_raw_uint88_t;
typedef BOOST_INT(96) sol_raw_int96_t;
typedef BOOST_UINT(96) sol_raw_uint96_t;
typedef BOOST_INT(104) sol_raw_int104_t;
typedef BOOST_UINT(104) sol_raw_uint104_t;
typedef BOOST_INT(112) sol_raw_int112_t;
typedef BOOST_UINT(112) sol_raw_uint112_t;
typedef BOOST_INT(120) sol_raw_int120_t;
typedef BOOST_UINT(120) sol_raw_uint120_t;
typedef BOOST_INT(128) sol_raw_int128_t;
typedef BOOST_UINT(128) sol_raw_uint128_t;
typedef BOOST_INT(136) sol_raw_int136_t;
typedef BOOST_UINT(136) sol_raw_uint136_t;
typedef BOOST_INT(144) sol_raw_int144_t;
typedef BOOST_UINT(144) sol_raw_uint144_t;
typedef BOOST_INT(152) sol_raw_int152_t;
typedef BOOST_UINT(152) sol_raw_uint152_t;
typedef BOOST_INT(160) sol_raw_int160_t;
typedef BOOST_UINT(160) sol_raw_uint160_t;
typedef BOOST_INT(168) sol_raw_int168_t;
typedef BOOST_UINT(168) sol_raw_uint168_t;
typedef BOOST_INT(176) sol_raw_int176_t;
typedef BOOST_UINT(176) sol_raw_uint176_t;
typedef BOOST_INT(184) sol_raw_int184_t;
typedef BOOST_UINT(184) sol_raw_uint184_t;
typedef BOOST_INT(192) sol_raw_int192_t;
typedef BOOST_UINT(192) sol_raw_uint192_t;
typedef BOOST_INT(200) sol_raw_int200_t;
typedef BOOST_UINT(200) sol_raw_uint200_t;
typedef BOOST_INT(208) sol_raw_int208_t;
typedef BOOST_UINT(208) sol_raw_uint208_t;
typedef BOOST_INT(216) sol_raw_int216_t;
typedef BOOST_UINT(216) sol_raw_uint216_t;
typedef BOOST_INT(224) sol_raw_int224_t;
typedef BOOST_UINT(224) sol_raw_uint224_t;
typedef BOOST_INT(232) sol_raw_int232_t;
typedef BOOST_UINT(232) sol_raw_uint232_t;
typedef BOOST_INT(240) sol_raw_int240_t;
typedef BOOST_UINT(240) sol_raw_uint240_t;
typedef BOOST_INT(248) sol_raw_int248_t;
typedef BOOST_UINT(248) sol_raw_uint248_t;
typedef BOOST_INT(256) sol_raw_int256_t;
typedef BOOST_UINT(256) sol_raw_uint256_t;
#elif defined MC_USE_STDINT
#include <stdint.h>
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
typedef int64_t sol_raw_int72_t;
typedef uint64_t sol_raw_uint72_t;
typedef int64_t sol_raw_int80_t;
typedef uint64_t sol_raw_uint80_t;
typedef int64_t sol_raw_int88_t;
typedef uint64_t sol_raw_uint88_t;
typedef int64_t sol_raw_int96_t;
typedef uint64_t sol_raw_uint96_t;
typedef int64_t sol_raw_int104_t;
typedef uint64_t sol_raw_uint104_t;
typedef int64_t sol_raw_int112_t;
typedef uint64_t sol_raw_uint112_t;
typedef int64_t sol_raw_int120_t;
typedef uint64_t sol_raw_uint120_t;
typedef int64_t sol_raw_int128_t;
typedef uint64_t sol_raw_uint128_t;
typedef int64_t sol_raw_int136_t;
typedef uint64_t sol_raw_uint136_t;
typedef int64_t sol_raw_int144_t;
typedef uint64_t sol_raw_uint144_t;
typedef int64_t sol_raw_int152_t;
typedef uint64_t sol_raw_uint152_t;
typedef int64_t sol_raw_int160_t;
typedef uint64_t sol_raw_uint160_t;
typedef int64_t sol_raw_int168_t;
typedef uint64_t sol_raw_uint168_t;
typedef int64_t sol_raw_int176_t;
typedef uint64_t sol_raw_uint176_t;
typedef int64_t sol_raw_int184_t;
typedef uint64_t sol_raw_uint184_t;
typedef int64_t sol_raw_int192_t;
typedef uint64_t sol_raw_uint192_t;
typedef int64_t sol_raw_int200_t;
typedef uint64_t sol_raw_uint200_t;
typedef int64_t sol_raw_int208_t;
typedef uint64_t sol_raw_uint208_t;
typedef int64_t sol_raw_int216_t;
typedef uint64_t sol_raw_uint216_t;
typedef int64_t sol_raw_int224_t;
typedef uint64_t sol_raw_uint224_t;
typedef int64_t sol_raw_int232_t;
typedef uint64_t sol_raw_uint232_t;
typedef int64_t sol_raw_int240_t;
typedef uint64_t sol_raw_uint240_t;
typedef int64_t sol_raw_int248_t;
typedef uint64_t sol_raw_uint248_t;
typedef int64_t sol_raw_int256_t;
typedef uint64_t sol_raw_uint256_t;
// TODO(scottwe): this should be 256 but we don't support it.
#define SOL_UINT256_MAX UINT64_MAX
#else
#error An integer model is required.
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declares the entry-point to the c-model.
void run_model(void);

// This method is called once, when the c-model is bootstrapping. It allows
// application-specific setup and allocations to be decoupled from the c-model.
// It also allows for arbitrary C++ code to be injected into the setup, without
// the knowledge of the c-model.
// 
// _argc and _argv are forwarded from main.
void sol_setup(int _argc, const char **_argv);

// This method is called on each iteration of the while loop. It returns zero if
// the loop should terminate.
uint8_t sol_continue(void);

// This method is called once before each transaction.
void sol_on_transaction(void);

// Placeholder calls for require() and assert() in solidity.
void sol_require(sol_raw_uint8_t _cond, const char* _msg);
void sol_assert(sol_raw_uint8_t cond, const char* _msg);

// Allows special behaviour on emit events.
void sol_emit(const char * _event);

// Raw call to assume, meant for use by the model's execution environment.
void ll_assume(sol_raw_uint8_t _cond);

// Returns a raw byte without any wrapping. This is meant to be used by the
// model's execution environment.
uint8_t nd_byte(const char* _msg);
uint8_t nd_range(uint8_t l, uint8_t u, const char* _msg);

// Method used to compute a value no less than than _curr. If _strict then the
// value must be larger.
sol_raw_uint256_t nd_increase(
    sol_raw_uint256_t _curr, uint8_t _strict, const char* _msg
);

// Allows logs from the model.
void smartace_log(const char* _msg);

// Provides non-deterministic integers for all native Solidity byte-widths.
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
