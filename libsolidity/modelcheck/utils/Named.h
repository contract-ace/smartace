/**
 * A simple named entity.
 * 
 * @date 2020
 */

#pragma once

#include <string>

namespace dev
{
namespace solidity
{
class Declaration;
}
}

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * A simple named entity.
 */
class Named
{
public:
    // Wraps the name of _decl.
    explicit Named(Declaration const& _decl);

    // Returns the name of this entity.
    std::string name() const;

private:
    std::string const M_NAME;
};

// -------------------------------------------------------------------------- //

}
}
}
