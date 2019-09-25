/**
 * Defines assert and require implementations for static analysis.
 * TODO(scottwe): sol_assert/sol_require may require more complicated behaviour
 *                later on... it would make sense for each to call into a "ll"
 *                implementation.
 * @date 2019
 */

#include "verify.h"

#include "seahorn/seahorn.h"

void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    sassert(_cond);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
    (void) _msg;
    assume(_cond);
}
