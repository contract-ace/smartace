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
    CMemberAccess(CExprPtr _expr, std::string _member);

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
    CCast(CExprPtr _expr, std::string _type);

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

/**
 * Represents a C block.
 */
class CBlock : public CStmt
{
public:
    // Wraps a list of statements inside a brace-enclosed block.
    CBlock(CBlockList _stmts);

private:
    CBlockList const m_stmts;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * The class of statements with reduce to a single expression.
 */
class CExprStmt : public CStmt
{
public:
    // Wraps the expression with a statement.
    CExprStmt(CExprPtr _expr);

private:
    CExprPtr const m_expr;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * The class of one variable declaration statements, with opt. initialization.
 */
class CVarDecl : public CStmt
{
public:
    // Declares a variable of given base type and name. It may be set as a
    // a pointer, adding * to the declaration, and may take an initial value.
    CVarDecl(std::string _type, std::string _name, bool _ptr, CExprPtr _init);

private:
    std::string const m_type;
    std::string const m_name;
    bool const m_ptr;
    CExprPtr const m_init;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * The class of if and if/else statements.
 */
class CIf : public CStmt
{
public:
    // Represents the statement if(_cond)_tsmtm[ else _fstmt], where _fstmt is
    // optional.
    CIf(CExprPtr _cond, CStmtPtr _tstmt, CStmtPtr _fstmt);

private:
    CExprPtr const m_cond;
    CStmtPtr const m_tstmt;
    CStmtPtr const m_fstmt;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to a while loop in C. Both the while and do/while variants.
 */
class CWhileLoop : public CStmt
{
public:
    // Constructs a while loop with given condition and body. If set to run at
    // least once, a do/while loop is generated.
    CWhileLoop(CStmtPtr _body, CExprPtr _cond, bool _atleast_once);

private:
    CStmtPtr const m_body;
    CExprPtr const m_cond;
    bool const m_dowhile;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to a for loop in C, in which all sub-stmts are optional.
 */
class CForLoop : public CStmt
{
public:
    // Constructs a while loop with given condition and body. If set to run at
    // least once, a do/while loop is generated.
    CForLoop(CStmtPtr _init, CExprPtr _cond, CStmtPtr _loop, CStmtPtr _body);

private:
    CStmtPtr const m_init;
    CExprPtr const m_cond;
    CStmtPtr const m_loop;
    CStmtPtr const m_body;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to a break statement in C.
 */
class CBreak : public CStmt
{
private:
    void print_impl(std::ostream & _out) const override;
};

/**
 * Corresponds to a break statement in C.
 */
class CContinue : public CStmt
{
private:
    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes a return call, with or without return value.
 */
class CReturn : public CStmt
{
public:
    // Creates a return statement with a return value.
    CReturn(CExprPtr _retval);

private:
    CExprPtr const m_retval;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

}
}
}
