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

/**
 * Extends the CElement class to handle statements and nested statements.
 */
class CStmt : public CElement
{
public:
    virtual ~CStmt() = default;

    void print(std::ostream & _o) const final;

    // Once called, the stmt will print itself as if it were a nested sub-stmt.
    void nest();

private:
    bool m_is_nested = false;

    // Prints the statement, modulo any ending tokens (ie semi-colons).
    virtual void print_impl(std::ostream & _out) const = 0;
};

class CVarDecl;
using CExprPtr = std::shared_ptr<CExpr>;
using CStmtPtr = std::shared_ptr<CStmt>;
using CArgList = std::vector<CExprPtr>;
using CBlockList = std::vector<CStmtPtr>;
using CParams = std::vector<std::shared_ptr<CVarDecl>>;

}
}
}
