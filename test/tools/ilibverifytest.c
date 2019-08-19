#include <libverify/verify.h>
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
    const int IS_ASSUME = (strcmp(TEST_OP, "assume") == 0);
    const int IS_REQURIE = (strcmp(TEST_OP, "require") == 0);
    const int IS_ND = (strcmp(TEST_OP, "nd") == 0);

    if (IS_ASSUME || IS_REQURIE)
    {
        if (argc < 3) { fail(); }
        long int const COND = strtol(argv[2], NULL, 10);
        char const* const MSG = ((argc >= 4) ? argv[3] : NULL);

        if (IS_ASSUME) { sol_assume(COND, MSG); }
        else { sol_require(COND, MSG); }
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
        case 4: return nd_uint8_t(MSG);
        case 5: return nd_uint16_t(MSG);
        case 6: return nd_uint32_t(MSG);
        case 7: return nd_uint64_t(MSG);
        default: fail();
        }
    }
    else { fail(); }

    return 0;
}
