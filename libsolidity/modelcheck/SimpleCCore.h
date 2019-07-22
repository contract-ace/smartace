/**
 * @date 2019
 * Declares the core interface to SimpleCGenerator.cpp
 */

#pragma once

#include <memory>
#include <ostream>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Base class for all C nodes. Streaming a CElement produces equivalent C code.
 */
class CElement
{
public:
    virtual ~CElement() = default;

    // Overriden to specify how a CElement should be formatted.
    virtual void print(std::ostream & _o) const = 0;
};

std::ostream & operator<<(std::ostream & _out, CElement const& _comp);

/**
 * Extends the CElement class to handle expression concerns.
 */
class CExpr : public CElement
{
public:
    virtual ~CExpr() = default;

    // Overriden to determine if an element is a pointer. Defaults to false.
    virtual bool is_pointer() const;
};

using CExprPtr = std::shared_ptr<CExpr>;
using CArgList = std::vector<CExprPtr>;

}
}
}
