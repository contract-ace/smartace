#include <libverify/verify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fail(void)
{
    fprintf(stderr, "Test failed.\n");
    exit(1);
}

int main(int argc, char const** argv)
{
    if (argc < 2) { fail(); }
    char const* const TEST_OP = argv[1];
    const int IS_ASSUME = (strcmp(TEST_OP, "assume") == 0);
    const int IS_REQURIE = (strcmp(TEST_OP, "require") == 0);

    if (IS_ASSUME || IS_REQURIE)
    {
        if (argc < 3) { fail(); }
        long int cond = strtol(argv[2], NULL, 10);
        char const* msg = ((argc >= 4) ? argv[3] : NULL);

        if (IS_ASSUME) { sol_assume(cond, msg); }
        else { sol_require(cond, msg); }
    }
    else { fail(); }

    return 0;
}
