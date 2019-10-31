/**
 * @date 2019
 * A limited, AST-like library specialized for transpiling Solidity into C.
 */

#pragma once

#include <libsolidity/modelcheck/SimpleCCore.h>
#include <map>
#include <string>
#include <type_traits>

namespace dev
{
namespace solidity
{

class Expression;
class Type;

namespace modelcheck
{

class TypeConverter;
class VariableScopeResolver;

// -------------------------------------------------------------------------- //

class CAssign;
class CMemberAccess;
class CIndexAccess;

/**
 * Defines an interface for accessible data.
 */
class CData
{
public:
    // Sets the value of this declaration to the given expression. This is
    // equivalent to CAssign(decl, rhs).
    std::shared_ptr<CAssign> assign(CExprPtr _rhs) const;

    // Similar to ID, except for the fact that a member access is returned.
    std::shared_ptr<CMemberAccess> access(std::string _member) const;

protected:
    // Returns the expr used in all interfaces.
    virtual CExprPtr expr() const = 0;
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

    ~CBinaryOp() = default;

    void print(std::ostream & _out) const override;

    // Converts this standalone call into a statement.
    CStmtPtr stmt();

private:
    CExprPtr const m_lhs;
    CExprPtr const m_rhs;
    std::string const m_op;
};

/**
 * Helper class to generate the (_lhs)=(_rhs) binary operation, due to its
 * frequency.
 */
class CAssign : public CBinaryOp
{
public:
    // Encodes the C expression (_lhs)_op(_rhs).
    CAssign(CExprPtr _lhs, CExprPtr _rhs);

    ~CAssign() = default;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes member access, both to pointers and to references.
 */
class CMemberAccess : public CExpr, public CData
{
public:
    // Encodes one of (_expr)._member or (_expr)->_member, based on context.
    CMemberAccess(CExprPtr _expr, std::string _member);

    ~CMemberAccess() = default;

    void print(std::ostream & _out) const override;

protected:
    CExprPtr expr() const override;

private:
    CExprPtr const m_expr;
    std::string const m_member;
};

// -------------------------------------------------------------------------- //

/**
 * Represents a named identifier in C.
 */
class CIdentifier : public CExpr, public CData
{
public:
    // Creates a CElement which generates an identifier equivalent to _name. As
    // AST scope analysis is not performed, _ptr declares if the referenced
    // declaration corresponds to a pointer.
    CIdentifier(std::string _name, bool _ptr);

    ~CIdentifier() = default;

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

protected:
    CExprPtr expr() const override;

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

    ~CIntLiteral() = default;

    void print(std::ostream & _out) const override;

private:
    long long int m_val;
};

// -------------------------------------------------------------------------- //

/**
 * Represents a literal, integer value in C.
 */
class CStringLiteral : public CExpr
{
public:
    // Creates a literal of some string.
    CStringLiteral(std::string const& _val);

    ~CStringLiteral() = default;

    void print(std::ostream & _out) const override;

private:
    std::string m_val;
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

    // Converts this standalone call into a statement.
    CStmtPtr stmt();

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
    ~CReference() = default;

    bool is_pointer() const override;
};

/**
 * Helper class to generate the *(_expr) unary operation, due to its frequency.
 */
class CDereference : public CUnaryOp
{
public:
    CDereference(CExprPtr _expr);
    ~CDereference() = default;
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

    ~CCond() = default;

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

    // Converts this standalone call into a statement.
    CStmtPtr stmt();

private:
    CExprPtr const m_cond;
    CExprPtr const m_tcase;
    CExprPtr const m_fcase;
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

    ~CCast() = default;

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

    ~CFuncCall() = default;

    void print(std::ostream & _out) const override;

    // Converts this standalone call into a statement.
    CStmtPtr stmt();

private:
    std::string const m_name;
    CArgList const m_args;
};

// Wraps the name of a function, and then allows calls to said function be built
// online.
class CFuncCallBuilder
{
public:
    // Creates a generator for calls to _name.
    CFuncCallBuilder(std::string _name);

    // Pushes _expr to the end of the argument list.
    void push(CExprPtr _expr, Type const* _t = nullptr);

    // Evaluates expression, taking into account if it is a wrapped type. The
    // arguments are forwarded to ExpressionConverter. If _t is set, and wraps a
    // basic type, then _expr is cast to that type. This captures the behaviour
    // of implicitly casting raw types.
    void push(
        Expression const& _expr,
        TypeConverter const& _converter,
        VariableScopeResolver const& _decls,
        bool _is_ref,
        Type const* _t = nullptr
    );

    // Creates a function call to the given name, using the pushed args. The
    // args are reset after initialization.
    std::shared_ptr<CFuncCall> merge_and_pop();

    CStmtPtr merge_and_pop_stmt();

private:
    std::string const m_name;
    CArgList m_args;

    CExprPtr wrap_with_type(CExprPtr && _expr, Type const& _t);
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

    ~CBlock() = default;

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

    ~CExprStmt() = default;

private:
    CExprPtr const m_expr;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * The class of one variable declaration statements, with opt. initialization.
 */
class CVarDecl : public CStmt, public CData
{
public:
    // Declares a variable of given base type and name. It may be set as a
    // pointer, adding * to the declaration, and may take an initial value.
    CVarDecl(std::string _type, std::string _name, bool _ptr, CExprPtr _init);
    CVarDecl(std::string _type, std::string _name, bool _ptr);
    CVarDecl(std::string _type, std::string _name);

    ~CVarDecl() = default;

    // Generates an identifier for this declaration.
    std::shared_ptr<CIdentifier> id() const;

protected:
    CExprPtr expr() const override;

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

    ~CIf() = default;

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

    ~CWhileLoop() = default;

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

    ~CForLoop() = default;

private:
    CStmtPtr const m_init;
    CExprPtr const m_cond;
    CStmtPtr const m_loop;
    CStmtPtr const m_body;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to an integral switch statement in C. Each case is scoped.
 */
class CSwitch : public CStmt
{
public:
    // Constructs a while loop with given condition and body. If set to run at
    // least once, a do/while loop is generated.
    CSwitch(CExprPtr _cond);
    CSwitch(CExprPtr _cond, CBlockList _default);

    void add_case(int64_t val, CBlockList body);

    ~CSwitch() = default;

private:
    CExprStmt m_cond;
    CBlock m_default;

    std::map<int64_t, CBlock> m_cases;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to a break statement in C.
 */
class CBreak : public CStmt
{
public:
    ~CBreak() = default;

private:
    void print_impl(std::ostream & _out) const override;
};

/**
 * Corresponds to a break statement in C.
 */
class CContinue : public CStmt
{
public:
    ~CContinue() = default;

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

    ~CReturn() = default;

private:
    CExprPtr const m_retval;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes the definition of a standard C function. The function may be made
 * a forward declaration by providing no body.
 */
class CFuncDef : public CElement
{
public:
    enum class Modifier { DEFAULT, INLINE };

    // Represents the function, _id.type _id.name(_args[0],...,args[k]){_body}.
    CFuncDef(
        std::shared_ptr<CVarDecl> _id,
        CParams _args,
        std::shared_ptr<CBlock> _body,
        Modifier _mod = Modifier::DEFAULT
    );

    void print(std::ostream & _out) const override;

private:
    std::shared_ptr<CVarDecl> m_id;
    CParams m_args;
    std::shared_ptr<CBlock> m_body;
    Modifier m_mod;
};

// -------------------------------------------------------------------------- //

/**
 * Generalizes a C typedef. The typedef is not verified for valid types.
 */
class CTypedef : public CElement
{
public:
    // Declares type _type as _name.
    CTypedef(std::string _type, std::string _name);

    void print(std::ostream & _out) const override;

private:
    std::string const m_type;
    std::string const m_name;
};

/**
 * Generalizes the definition of a standard C structure. This function may be
 * made a forward declaration by providing no body.
 */
class CStructDef : public CElement
{
public:
    // Represents the structure, struct _name { _fields[0];,...,fields[k]; };.
    CStructDef(std::string _name, std::shared_ptr<CParams> _fields);

    void print(std::ostream & _out) const override;

    // Automatically generates a typedef for this structure, with symbol _name.
    std::shared_ptr<CTypedef> make_typedef(std::string _name);

    // Automatically generates a CVarDecl of this type.
    std::shared_ptr<CVarDecl> decl(std::string _name);
    std::shared_ptr<CVarDecl> decl(std::string _name, bool _ptr);

private:
    std::string m_name;
    std::shared_ptr<CParams> m_fields;
};

// -------------------------------------------------------------------------- //

}
}
}
