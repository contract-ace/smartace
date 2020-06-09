/**
 * Compiler-specific preprocessor macros.
 * 
 * @date 2020
 */

#pragma once

#include <boost/predef.h>

// -------------------------------------------------------------------------- //

// Disables the warning named `warn` in Clang.
// Expected usage:
//     CLANG_SUPPRCESS_COMPILER_WARNING_CLANG(CLANG_OVERLOADED_VIRTUAL)
//     violating_code();
//     CLANG_ENABLE_COMPILER_WARNING()

#define CLANG_OVERLOADED_VIRTUAL \
    "clang diagnostic ignored \"-Woverloaded-virtual\""

#if BOOST_COMP_CLANG

#define CLANG_SUPPRCESS_COMPILER_WARNING_CLANG(warn) \
    _Pragma("clang diagnostic push") \
    _Pragma (warn)

#define CLANG_ENABLE_COMPILER_WARNING() \
    _Pragma("clang diagnostic pop")

#else
#define CLANG_SUPPRCESS_COMPILER_WARNING_CLANG(warn)
#define CLANG_ENABLE_COMPILER_WARNING()
#endif

// -------------------------------------------------------------------------- //
