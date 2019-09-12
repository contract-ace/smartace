/**
 * This utility will generate sol_verify.h.
 * @date 2019
 */

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 2) return -1;
    ofstream output(argv[1]);

    // Headers and pragmas.
    output << "#pragma once" << endl;
    output << "#include \"gmp.h\"" << endl;
    output << "#include <stdint.h>" << endl;

    // Assertations.
    output << "void sol_require(uint8_t _cond, const char* _msg);";
    output << "void sol_assert(uint8_t cond, const char* _msg);";

    // ND values.
    for (unsigned int i = 8; i <= 64; i *= 2)
    {
        output << "int" << i << "_t nd_int" << i << "_t(const char* _msg);";
        output << "uint" << i << "_t nd_uint" << i << "_t(const char* _msg);";
    }
    output << "__int128_t nd_int128_t(const char* _msg);";
    output << "void nd_int256_t(mpz_t _dest, const char* _msg);";
    output << "__uint128_t nd_uint128_t(const char* _msg);";
    output << "void nd_uint256_t(mpz_t _dest, const char* _msg);";

    return 0;
}
