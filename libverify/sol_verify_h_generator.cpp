/**
 * This utility will generate sol_verify.h.
 * @date 2019
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// -------------------------------------------------------------------------- //

// The macro lookup generates macros of the form {BASE}_(u)INT{n}. This
// interface ensures that macros are used consistently across this file.
class MacroNameLookup
{
public:
    MacroNameLookup(string _base): M_BASE(move(_base)) {}

    string unsigned_sym(uint16_t n) const { return _lookup(false, n); }

    string signed_sym(uint16_t n) const { return _lookup(true, n); }

private:
    const string M_BASE;

    string _lookup(bool is_signed, uint16_t n) const
    {
        ostringstream oss;
        oss << M_BASE << "_" << (is_signed ? "INT" : "UINT") << n;
        return oss.str();
    }
};

// -------------------------------------------------------------------------- //

// Defines a type which produces an stdint type for any byte aligned, power of 2
// bitwidth. 128-bit types are achieved using __(u)int128_t. For bitwidths
// larger than 128 bits, the type is rounded down to __(u)int128_t.
class StdIntGenerator
{
public:
    StdIntGenerator(bool _sgnd, uint16_t _bits): M_SGND(_sgnd), M_BITS(_bits) {}

private:
    bool M_SGND;
    uint16_t M_BITS;

    friend ostream & operator<<(ostream& os, const StdIntGenerator GEN);
};

ostream & operator<<(ostream & os, const StdIntGenerator GEN)
{
    if (GEN.M_BITS > 64) os << "__";
    if (!GEN.M_SGND) os << "u";
    os << "int" << GEN.M_BITS << "_t";
    return os;
}

// -------------------------------------------------------------------------- //

// Uses stdint, and generates a macro definition for each bitwidth type.
void define_raw_types(ostream & output, MacroNameLookup const& LOOKUP)
{
    uint16_t bits_required = 8;
    for (uint16_t bits = 8; bits <= 256; bits += 8)
    {
        const string TYPE_PREFIX = (bits <= 64 ? "" : "__");

        output << "#define " << LOOKUP.unsigned_sym(bits)
               << " " << StdIntGenerator(false, bits_required) << endl;
        output << "#define " << LOOKUP.signed_sym(bits)
               << " " << StdIntGenerator(true, bits_required) << endl;

        if (bits_required == bits && bits_required < 128)
        {
            bits_required *= 2;
        }
    }
}

// -------------------------------------------------------------------------- //

int main(int argc, char *argv[])
{
    const MacroNameLookup MACRO_LOOKUP("SOL_INTEGER");

    if (argc != 2) return -1;
    ofstream output(argv[1]);

    // Headers and pragmas.
    output << "#pragma once" << endl;
    output << "#include <stdint.h>" << endl;

    // Declares types for each build.
    output << "#ifdef MC_USE_BOOST_MP" << endl;
    output << "#include <boost/multiprecision/cpp_int.hpp>" << endl;
    define_raw_types(output, MACRO_LOOKUP);
    output << "#elif defined MC_USE_STDINT" << endl;
    define_raw_types(output, MACRO_LOOKUP);
    output << "#endif" << endl;

    // Ensures this can link to C.
    output << "#ifdef __cplusplus" << endl;
    output << "extern \"C\" {" << endl;
    output << "#endif" << endl;

    // Assertations.
    output << "void sol_require("
           << MACRO_LOOKUP.unsigned_sym(8) <<  " _cond, const char* _msg);";
    output << "void sol_assert("
           << MACRO_LOOKUP.unsigned_sym(8) << " cond, const char* _msg);";

    // ND values.
    for (unsigned int bits = 8; bits <= 256; bits *= 2)
    {
        output << MACRO_LOOKUP.signed_sym(bits)
               << " nd_int" << bits << "_t(const char* _msg);";
        output << MACRO_LOOKUP.unsigned_sym(bits)
               << " nd_uint" << bits << "_t(const char* _msg);"; 
    }

    // Ends C interface.
    output << endl << "#ifdef __cplusplus" << endl;
    output << "}" << endl;
    output << "#endif" << endl;

    return 0;
}

// -------------------------------------------------------------------------- //
