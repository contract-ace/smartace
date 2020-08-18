/**
 * Maps Solidity primitive types to SmartACE raw types.
 * 
 * @date 2020
 */

#pragma once

#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * The class provides an analysis tool, which will determine all primitive types
 * in use, and then produce the primitive type declarations required to
 * translate the given AST into C. 
 */
class PrimitiveToRaw
{
public:
    // Returns the raw type used to represented _width bit integers. If _signed
    // is true then the integer is signed, otherwise it is unsigned.
    static std::string integer(uint16_t _width, bool _signed);

    // Returns the raw type used to represent booleans.
    static std::string boolean();

    // Returns the raw type used to represent addresses.
    static std::string address();
};

// -------------------------------------------------------------------------- //

}
}
}
