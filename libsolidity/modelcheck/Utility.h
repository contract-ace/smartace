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

}
}
}
