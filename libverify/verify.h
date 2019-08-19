/**
 * Declares an interface for the Solidity model-checking runtime.
 * @date 2019
 */

#pragma once

#include <stdint.h>

// Stands in for `require(_cond, _msg)` in Solidity.
void sol_require(uint8_t _cond, const char* _msg);

// Stands in for `assume(_cond, _msg)` in Solidity.
void sol_assume(uint8_t cond, const char* _msg);
