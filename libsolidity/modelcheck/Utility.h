/**
 * @date 2019
 * A toolkit of low-level, general utilities for use across all translation
 * untis.
 */

#pragma once

#include <type_traits>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

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
    static_assert(std::is_copy_constructible<T>::value);

    // To reset the value, T must be copy assignable.
    static_assert(std::is_copy_assignable<T>::value);

    // Sets the value at _ref to _set until this object goes out of scope.
	ScopedSwap(T &_ref, T _set): M_INIT(_ref), m_ref(_ref) { m_ref = _set; }

	~ScopedSwap() { m_ref = M_INIT; }

    // Allows access to the old value.
	T const& old() { return M_INIT; }

private:
	T const M_INIT;
	T &m_ref;
};

/**
 * Determines if _x is a power of 2. Clearly T must be integral for powers to be
 * defined. This is valid only positive powers of 2.
 */
template <typename T>
inline bool is_power_of_two(T _x)
{
    static_assert(std::is_integral<T>::value);
    // If _x is 2^n, then _x has binary representation b100...<n-1>...0. Then
    // (_x - 1) has binary representation b11...<n-1>...1. Then x & (x - 1) must
    // be 0. When _x is not a power of two, one bit after the leading bit must
    // be 1. This gives that x & (x - 1) is not 0. It is ensured that _x > 0, as
    // 1. there does not exist a real n such that 2^n < 0
    // 2. there exists _x less than 1 such that _x & (_x - 1) = 0.
    return (_x > 0) && ((_x & (_x - 1)) == 0);
}

}
}
}
