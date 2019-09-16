#include "libverify/verify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fail(void)
{
    fprintf(stderr, "Test failed.\n");
    exit(-1);
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
        case 2: return nd_int24_t(MSG);
        case 3: return nd_int32_t(MSG);
        case 4: return nd_int40_t(MSG);
        case 5: return nd_int48_t(MSG);
        case 6: return nd_int56_t(MSG);
        case 7: return nd_int64_t(MSG);
        case 8: return nd_int72_t(MSG);
        case 9: return nd_int80_t(MSG);
        case 10: return nd_int88_t(MSG);
        case 11: return nd_int96_t(MSG);
        case 12: return nd_int104_t(MSG);
        case 13: return nd_int112_t(MSG);
        case 14: return nd_int120_t(MSG);
        case 15: return nd_int128_t(MSG);
        case 16: return nd_int136_t(MSG);
        case 17: return nd_int144_t(MSG);
        case 18: return nd_int152_t(MSG);
        case 19: return nd_int160_t(MSG);
        case 20: return nd_int168_t(MSG);
        case 21: return nd_int176_t(MSG);
        case 22: return nd_int184_t(MSG);
        case 23: return nd_int192_t(MSG);
        case 24: return nd_int200_t(MSG);
        case 25: return nd_int208_t(MSG);
        case 26: return nd_int216_t(MSG);
        case 27: return nd_int224_t(MSG);
        case 28: return nd_int232_t(MSG);
        case 29: return nd_int240_t(MSG);
        case 30: return nd_int248_t(MSG);
        case 31: return nd_int256_t(MSG);
        case 32: return nd_uint8_t(MSG);
        case 33: return nd_uint16_t(MSG);
        case 34: return nd_uint24_t(MSG);
        case 35: return nd_uint32_t(MSG);
        case 36: return nd_uint40_t(MSG);
        case 37: return nd_uint48_t(MSG);
        case 38: return nd_uint56_t(MSG);
        case 39: return nd_uint64_t(MSG);
        case 40: return nd_uint72_t(MSG);
        case 41: return nd_uint80_t(MSG);
        case 42: return nd_uint88_t(MSG);
        case 43: return nd_uint96_t(MSG);
        case 44: return nd_uint104_t(MSG);
        case 45: return nd_uint112_t(MSG);
        case 46: return nd_uint120_t(MSG);
        case 47: return nd_uint128_t(MSG);
        case 48: return nd_uint136_t(MSG);
        case 49: return nd_uint144_t(MSG);
        case 50: return nd_uint152_t(MSG);
        case 51: return nd_uint160_t(MSG);
        case 52: return nd_uint168_t(MSG);
        case 53: return nd_uint176_t(MSG);
        case 54: return nd_uint184_t(MSG);
        case 55: return nd_uint192_t(MSG);
        case 56: return nd_uint200_t(MSG);
        case 57: return nd_uint208_t(MSG);
        case 58: return nd_uint216_t(MSG);
        case 59: return nd_uint224_t(MSG);
        case 60: return nd_uint232_t(MSG);
        case 61: return nd_uint240_t(MSG);
        case 62: return nd_uint248_t(MSG);
        case 63: return nd_uint256_t(MSG);
        default: fail();
        }
    }
    else { fail(); }

    return 0;
}
