#include <libsolidity/modelcheck/utils/KeyIterator.h>

#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

KeyIterator::KeyIterator(
    size_t _width, size_t _depth
): M_WIDTH(_width), M_DEPTH(_depth) { }

// -------------------------------------------------------------------------- //

string KeyIterator::suffix() const
{
    string suffix;
    for (auto idx : m_indices) suffix += "_" + to_string(idx);
    return suffix;
}

// -------------------------------------------------------------------------- //

bool KeyIterator::is_full() const
{
    return (M_DEPTH > 0 && m_indices.size() == M_DEPTH);
}

// -------------------------------------------------------------------------- //

size_t KeyIterator::size() const
{
    return m_indices.size();
}

// -------------------------------------------------------------------------- //

bool KeyIterator::next()
{
    if (M_WIDTH == 0 || M_DEPTH == 0)
    {
        return false;
    }
    else if (!is_full())
    {
        m_indices.push_back(0);
    }
    else
    {
        ++m_indices.back();
        while (m_indices.back() == M_WIDTH)
        {
            m_indices.pop_back();
            if (m_indices.empty()) break;
            ++m_indices.back();
        }
    }

    return !m_indices.empty();
}

// -------------------------------------------------------------------------- //

}
}
}
