/**
 * @date 2019
 * A limited, AST-like library specialized for transpiling Solidity into C.
 */

#pragma once

#include <libsolidity/modelcheck/SimpleCCore.h>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Represents a named identifier in C.
 */
class CIdentifier : public CExpr
{
public:
    // Creates a CElement which generates an identifier equivalent to _name. As
    // AST scope analysis is not performed, _ptr declares if the referenced
    // declaration corresponds to a pointer.
    CIdentifier(std::string _name, bool _ptr);

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

private:
    std::string const m_name;
    bool m_ptr;
};

// -------------------------------------------------------------------------- //

/**
 * Represents a literal, integer value in C.
 */
class CIntLiteral : public CExpr
{
public:
    // Creates a literal of signed value _val.
    CIntLiteral(long long int _val);

    void print(std::ostream & _out) const override;

private:
    long long int m_val;
};

// -------------------------------------------------------------------------- //

/**
 * Generalization of a prefix or suffix unary operator.
 */
class CUnaryOp : public CExpr
{
public:
    // Encodes either C-expression _op(_expr) or (_expr)_op, dictated by _pre.
    CUnaryOp(std::string _op, CExprPtr _expr, bool _pre);

    virtual ~CUnaryOp() = default;

    void print(std::ostream & _out) const override;

private:
    std::string const m_op;
    std::shared_ptr<CExpr> const m_expr;
    bool const m_pre;
};

/**
 * Helper class to generate the &(_expr) unary operation, due to its frequency.
 */
class CReference : public CUnaryOp
{
public:
    CReference(CExprPtr _expr);

    bool is_pointer() const override;
};

/**
 * Helper class to generate the *(_expr) unary operation, due to its frequency.
 */
class CDereference : public CUnaryOp
{
public:
    CDereference(CExprPtr _expr);
};

// -------------------------------------------------------------------------- //

/**
 * Generalization of a binary operator.
 */
class CBinaryOp : public CExpr
{
public:
    // Encodes the C expression (_lhs)_op(_rhs).
    CBinaryOp(CExprPtr _lhs, std::string _op, CExprPtr _rhs);

    void print(std::ostream & _out) const override;

private:
    CExprPtr const m_lhs;
    CExprPtr const m_rhs;
    std::string const m_op;
};

// -------------------------------------------------------------------------- //

/**
 * Generializes a conditional C statement.
 */
class CCond : public CExpr
{
public:
    // Encodes the C expression (_cond)?(_tcase):(_fcase).
    CCond(CExprPtr _cond, CExprPtr _tcase, CExprPtr _fcase);

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

private:
    CExprPtr const m_cond;
    CExprPtr const m_tcase;
    CExprPtr const m_fcase;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes member access, both to pointers and to references.
 */
class CMemberAccess : public CExpr
{
public:
    // Encodes one of (_expr)._member or (_expr)->_member, based on context.
    CMemberAccess(CExprPtr const _expr, std::string _member);

    void print(std::ostream & _out) const override;

private:
    CExprPtr const m_expr;
    std::string const m_member;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes an inline, expliict cast.
 */
class CCast : public CExpr
{
public:
    // Encodes the C expression ((_type)(_expr)).
    CCast(CExprPtr const _expr, std::string _type);

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

private:
    CExprPtr const m_expr;
    std::string const m_type;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes a function call, without support for function pointers.
 */
class CFuncCall : public CExpr
{
public:
    // Encodes the C expression _name(_args[0],_args[1],...,args[n]).
    CFuncCall(std::string _name, CArgList _args);

    void print(std::ostream & _out) const override;

private:
    std::string const m_name;
    CArgList const m_args;
};

// -------------------------------------------------------------------------- //

}
}
}
