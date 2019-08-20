/**
 * Defines an interactive implementation of assume and require. When either
 * assertation fails, the execution is halted, the reason is logged. If the
 * assertion was a requirement, then a non-zero return value is produced.
 * @date 2019
 */

#include "verify.h"

#include <stdio.h>
#include <stdlib.h>

void sol_assertion_impl(
    int _status, uint8_t _cond, char const* _check, char const* _msg
)
{
    if (!_cond)
    {
        fprintf(stderr, "%s", _check);
        if (_msg) fprintf(stderr, ": %s", _msg);
        fprintf(stderr, "\n");
        exit(_status);
    }
}

void sol_assert(uint8_t _cond, const char* _msg)
{
    sol_assertion_impl(-1, _cond, "assert", _msg);
}

void sol_require(uint8_t _cond, const char* _msg)
{
    sol_assertion_impl(0, _cond, "require", _msg);
}
