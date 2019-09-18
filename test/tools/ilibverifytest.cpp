#include "libverify/verify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MC_USE_BOOST_MP
#define DISPLAY_ND_INT(RAW) (RAW).template convert_to<int>();
#else
#define DISPLAY_ND_INT(RAW) RAW
#endif

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
        case 0: return DISPLAY_ND_INT(nd_int8_t(MSG));
        case 1: return DISPLAY_ND_INT(nd_int16_t(MSG));
        case 2: return DISPLAY_ND_INT(nd_int24_t(MSG));
        case 3: return DISPLAY_ND_INT(nd_int32_t(MSG));
        case 4: return DISPLAY_ND_INT(nd_int40_t(MSG));
        case 5: return DISPLAY_ND_INT(nd_int48_t(MSG));
        case 6: return DISPLAY_ND_INT(nd_int56_t(MSG));
        case 7: return DISPLAY_ND_INT(nd_int64_t(MSG));
        case 8: return DISPLAY_ND_INT(nd_int72_t(MSG));
        case 9: return DISPLAY_ND_INT(nd_int80_t(MSG));
        case 10: return DISPLAY_ND_INT(nd_int88_t(MSG));
        case 11: return DISPLAY_ND_INT(nd_int96_t(MSG));
        case 12: return DISPLAY_ND_INT(nd_int104_t(MSG));
        case 13: return DISPLAY_ND_INT(nd_int112_t(MSG));
        case 14: return DISPLAY_ND_INT(nd_int120_t(MSG));
        case 15: return DISPLAY_ND_INT(nd_int128_t(MSG));
        case 16: return DISPLAY_ND_INT(nd_int136_t(MSG));
        case 17: return DISPLAY_ND_INT(nd_int144_t(MSG));
        case 18: return DISPLAY_ND_INT(nd_int152_t(MSG));
        case 19: return DISPLAY_ND_INT(nd_int160_t(MSG));
        case 20: return DISPLAY_ND_INT(nd_int168_t(MSG));
        case 21: return DISPLAY_ND_INT(nd_int176_t(MSG));
        case 22: return DISPLAY_ND_INT(nd_int184_t(MSG));
        case 23: return DISPLAY_ND_INT(nd_int192_t(MSG));
        case 24: return DISPLAY_ND_INT(nd_int200_t(MSG));
        case 25: return DISPLAY_ND_INT(nd_int208_t(MSG));
        case 26: return DISPLAY_ND_INT(nd_int216_t(MSG));
        case 27: return DISPLAY_ND_INT(nd_int224_t(MSG));
        case 28: return DISPLAY_ND_INT(nd_int232_t(MSG));
        case 29: return DISPLAY_ND_INT(nd_int240_t(MSG));
        case 30: return DISPLAY_ND_INT(nd_int248_t(MSG));
        case 31: return DISPLAY_ND_INT(nd_int256_t(MSG));
        case 32: return DISPLAY_ND_INT(nd_uint8_t(MSG));
        case 33: return DISPLAY_ND_INT(nd_uint16_t(MSG));
        case 34: return DISPLAY_ND_INT(nd_uint24_t(MSG));
        case 35: return DISPLAY_ND_INT(nd_uint32_t(MSG));
        case 36: return DISPLAY_ND_INT(nd_uint40_t(MSG));
        case 37: return DISPLAY_ND_INT(nd_uint48_t(MSG));
        case 38: return DISPLAY_ND_INT(nd_uint56_t(MSG));
        case 39: return DISPLAY_ND_INT(nd_uint64_t(MSG));
        case 40: return DISPLAY_ND_INT(nd_uint72_t(MSG));
        case 41: return DISPLAY_ND_INT(nd_uint80_t(MSG));
        case 42: return DISPLAY_ND_INT(nd_uint88_t(MSG));
        case 43: return DISPLAY_ND_INT(nd_uint96_t(MSG));
        case 44: return DISPLAY_ND_INT(nd_uint104_t(MSG));
        case 45: return DISPLAY_ND_INT(nd_uint112_t(MSG));
        case 46: return DISPLAY_ND_INT(nd_uint120_t(MSG));
        case 47: return DISPLAY_ND_INT(nd_uint128_t(MSG));
        case 48: return DISPLAY_ND_INT(nd_uint136_t(MSG));
        case 49: return DISPLAY_ND_INT(nd_uint144_t(MSG));
        case 50: return DISPLAY_ND_INT(nd_uint152_t(MSG));
        case 51: return DISPLAY_ND_INT(nd_uint160_t(MSG));
        case 52: return DISPLAY_ND_INT(nd_uint168_t(MSG));
        case 53: return DISPLAY_ND_INT(nd_uint176_t(MSG));
        case 54: return DISPLAY_ND_INT(nd_uint184_t(MSG));
        case 55: return DISPLAY_ND_INT(nd_uint192_t(MSG));
        case 56: return DISPLAY_ND_INT(nd_uint200_t(MSG));
        case 57: return DISPLAY_ND_INT(nd_uint208_t(MSG));
        case 58: return DISPLAY_ND_INT(nd_uint216_t(MSG));
        case 59: return DISPLAY_ND_INT(nd_uint224_t(MSG));
        case 60: return DISPLAY_ND_INT(nd_uint232_t(MSG));
        case 61: return DISPLAY_ND_INT(nd_uint240_t(MSG));
        case 62: return DISPLAY_ND_INT(nd_uint248_t(MSG));
        case 63: return DISPLAY_ND_INT(nd_uint256_t(MSG));
        default: fail();
        }
    }
    else { fail(); }

    return 0;
}
