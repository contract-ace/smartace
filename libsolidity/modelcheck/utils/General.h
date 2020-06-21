/**
 * A toolkit of simple and general utilities for use across all modules.
 * 
 * @date 2019
 */

#pragma once

#include <stdexcept>
#include <type_traits>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * A RAII-like data structure which will consume a reference to a variable, copy
 * its value, and then overwrite it with a new value. On destruction, the old
 * value will be restored.
 */
template <typename T>
class ScopedSwap
{
public:
    // To retain the old value, T must be copy constructible.
    static_assert(
        std::is_copy_constructible<T>::value,
        "This swap is implemented through copy construction of type T."
    );

    // To reset the value, T must be copy assignable.
    static_assert(
        std::is_copy_assignable<T>::value,
        "This swap is implemented through copy assignment of type T."
    );

    // Sets the value at _ref to _set until this object goes out of scope.
	ScopedSwap(T &_ref, T _set): M_INIT(_ref), m_ref(_ref) { m_ref = _set; }

	~ScopedSwap() { m_ref = M_INIT; }

    // Allows access to the old value.
	T const& old() { return M_INIT; }

private:
	T const M_INIT;
	T &m_ref;
};

// -------------------------------------------------------------------------- //

/**
 * Computes an integer power (_base)^(_exp).
 */
inline uint64_t fast_pow(uint64_t _base, uint64_t _exp)
{
    if (_base == 0) return 0;
    if (_base == 1) return 1;

    uint64_t res = 1;
    while (_exp > 0)
    {
        if (_exp % 2 == 0)
        {
            _exp /= 2;
            _base *= _base;
        }
        else
        {
            res *= _base;
            _exp -= 1;
        }
    }
    return res;
}

// -------------------------------------------------------------------------- //

}
}
}
