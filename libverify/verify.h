/**
 * Declares an interface for the Solidity model-checking runtime.
 * @date 2019
 */

#pragma once

#include "gmp.h"
#include <stdint.h>

// Stands in for `require(_cond, _msg)` in Solidity.
void sol_require(uint8_t _cond, const char* _msg);

// Stands in for `assert(_cond, _msg)` in Solidity.
void sol_assert(uint8_t cond, const char* _msg);

// Returns non-deterministic values of different lengths.
int8_t nd_int8_t(const char* _msg);
int16_t nd_int16_t(const char* _msg);
int32_t nd_int32_t(const char* _msg);
int64_t nd_int64_t(const char* _msg);
__int128_t nd_int128_t(const char* _msg);
void nd_int256_t(mpz_t _dest, const char* _msg);
uint8_t nd_uint8_t(const char* _msg);
uint16_t nd_uint16_t(const char* _msg);
uint32_t nd_uint32_t(const char* _msg);
uint64_t nd_uint64_t(const char* _msg);
__uint128_t nd_uint128_t(const char* _msg);
void nd_uint256_t(mpz_t _dest, const char* _msg);
