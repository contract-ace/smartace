/**
 * Utilities to explore a multi-dimensional key space. 
 * 
 * @date 2021
 */

#pragma once

#include <cstdint>
#include <list>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utility to iterate all key combinations.
 */
class KeyIterator
{
public:
    // Creates an iterator over all length 1 to length _depth strings over
    // the alphabet [_width_offset, 1, ..., _width-1].
    KeyIterator(size_t _width, size_t _depth, size_t _width_offset);

    // Creates an iterator over all length 1 to length _depth strings over
    // the alphabet [0, 1, ..., _width-1].
    KeyIterator(size_t _width, size_t _depth);

    // Returns the current string as (s_0)_(s_2)_..._(s_{i}), where i is
    // some length from 1 to _depth. This value changes only after a call to
    // next.
    std::string suffix() const;

    // Returns true if the current suffix is of maximum length.
    bool is_full() const;

    // Returns the current suffix length.
    size_t size() const;

    // Updates all view fields. Returns true if there is a suffix left to iterate.
    bool next();
    
private:
    // The dimensions of the search space.
    size_t M_WIDTH;
    size_t M_DEPTH;
    size_t M_WIDTH_OFFSET;

    // The current string.
    std::list<size_t> m_indices;
};

// -------------------------------------------------------------------------- //

}
}
}
