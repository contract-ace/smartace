/**
 * Utilities to explore a multi-dimensional key space. 
 * 
 * @date 2021
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

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
    // the alphabet [0, 1, ..., _include]. The string is full only if the symbol
    // _include appears at lesat once.
    KeyIterator(size_t _width, size_t _depth, size_t _include);

    // Creates an iterator over all length 1 to length _depth strings over
    // the alphabet [0, 1, ..., _width-1].
    KeyIterator(size_t _width, size_t _depth);

    // Returns the current string as (s_0)_(s_2)_..._(s_{i}), where i is
    // some length from 1 to _depth. This value changes only after a call to
    // next.
    std::string suffix() const;

    // Returns true if the current suffix is of maximum length and valid.
    bool is_full() const;

    // Returns the current suffix length.
    size_t size() const;

    // Returns call indices.
    std::vector<size_t> const& view() const;

    // Advances indices. Returns true if one or more valid suffixes remain.
    bool next();
    
private:
    // Returns true if the current suffix is of maximum length.
    bool is_at_max() const;

    // The dimensions of the search space.
    size_t M_WIDTH;
    size_t M_DEPTH;
    size_t M_INCLUDE;

    // The current string.
    // - The stl vector does not deallocate memory so all operations are O(1).
    // - This is more efficient than a list, as it does not require allocations.
    std::vector<size_t> m_indices;
};

// -------------------------------------------------------------------------- //

}
}
}
