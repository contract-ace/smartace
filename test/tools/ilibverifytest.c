#include <libverify/verify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fail(void)
{
    fprintf(stderr, "Test failed.\n");
    exit(-1);
}

int multipercision_nd(const char* _msg, long int _case)
{
    mpz_t val;
    mpz_init(val);
    switch (_case)
    {
    case 5: nd_int256_t(val, _msg); break;
    case 11: nd_uint256_t(val, _msg); break;
    default: fail();
    }
    return mpz_get_si(val);
}

int main(int argc, char const** argv)
{
    if (argc < 2) { fail(); }
    char const* const TEST_OP = argv[1];
    const int IS_REQUIRE = (strcmp(TEST_OP, "require") == 0);
    const int IS_ASSERT = (strcmp(TEST_OP, "assert") == 0);
    const int IS_ND = (strcmp(TEST_OP, "nd") == 0);

    if (IS_REQUIRE || IS_ASSERT)
    {
        if (argc < 3) { fail(); }
        long int const COND = strtol(argv[2], NULL, 10);
        char const* const MSG = ((argc >= 4) ? argv[3] : NULL);

        if (IS_REQUIRE) { sol_require(COND, MSG); }
        else { sol_assert(COND, MSG); }
    }
    else if (IS_ND)
    {
        if (argc != 4) { fail(); }
        long int const TYPE = strtol(argv[2], NULL, 10);
        char const* const MSG = argv[3];

        switch (TYPE)
        {
        case 0: return nd_int8_t(MSG);
        case 1: return nd_int16_t(MSG);
        case 2: return nd_int32_t(MSG);
        case 3: return nd_int64_t(MSG);
        case 4: return nd_int128_t(MSG);
        case 6: return nd_uint8_t(MSG);
        case 7: return nd_uint16_t(MSG);
        case 8: return nd_uint32_t(MSG);
        case 9: return nd_uint64_t(MSG);
        case 10: return nd_uint128_t(MSG);
        default: return multipercision_nd(MSG, TYPE);
        }
    }
    else { fail(); }

    return 0;
}
