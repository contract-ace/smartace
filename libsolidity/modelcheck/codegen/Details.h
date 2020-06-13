/**
 * A limited, AST-like library specialized for transpiling Solidity into C.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/modelcheck/codegen/Core.h>

#include <map>
#include <string>

namespace dev
{
namespace solidity
{

class Expression;
class Type;

namespace modelcheck
{

class AnalysisStack;
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
 * Represents an inline C comment.
 */
class CComment : public CStmt
{
public:
    // Creates a literal of signed value _val.
    CComment(std::string _val);

    ~CComment() = default;

    void print_impl(std::ostream & _out) const override;

private:
    std::string m_val;
};

// -------------------------------------------------------------------------- //

/**
 * A template for a binary operator.
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
    CExprPtr const M_LHS;
    CExprPtr const M_RHS;
    std::string const M_OP;
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
 * A template for member accesses, both to pointers and to references.
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
    CExprPtr const M_EXPR;
    std::string const M_MEMBER;
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
    std::string const M_NAME;
    bool const M_IS_PTR;
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
    long long int const M_VAL;
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
    std::string const M_VAL;

    // Helper method to escape the string _val.
    static std::string escape_cstring(std::string _val);
};

// -------------------------------------------------------------------------- //

/**
 * A template for a prefix or suffix unary operator.
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
    std::string const M_OP;
    std::shared_ptr<CExpr> const M_EXPR;
    bool const M_PRE;
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
 * A template for a conditional c-statement.
 */
class CCond : public CExpr
{
public:
    // Encodes the C expression (_cond)?(_true_case):(_false_case).
    CCond(CExprPtr _cond, CExprPtr _true_case, CExprPtr _false_case);

    ~CCond() = default;

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

    // Converts this standalone call into a statement.
    CStmtPtr stmt();

private:
    CExprPtr const M_COND;
    CExprPtr const M_TRUE_CASE;
    CExprPtr const M_FALSE_CASE;
};

// -------------------------------------------------------------------------- //

/**
 * A template for an inline, explicit cast.
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
    CExprPtr const M_EXPR;
    std::string const M_TYPE;
};

// -------------------------------------------------------------------------- //

/**
 * A template for a function call, without support for function pointers.
 */
class CFuncCall : public CExpr
{
public:
    // Encodes the C expression _name(_args[0],_args[1],...,args[n]).
    CFuncCall(std::string _name, CArgList _args, bool _rv_is_ref = false);

    ~CFuncCall() = default;

    void print(std::ostream & _out) const override;
    bool is_pointer() const override;

    // Converts this standalone call into a statement.
    CStmtPtr stmt();

private:
    std::string const M_NAME;
    bool const M_RV_IS_REF;
    CArgList const M_ARGS;
};

/**
 *  Wraps the name of a function, and then allows calls to said function be built
 * online.
 */
class CFuncCallBuilder
{
public:
    // Creates a generator for calls to _name.
    CFuncCallBuilder(std::string _name, bool _rv_is_ref = false);

    // Pushes _expr to the end of the argument list.
    void push(CExprPtr _expr, Type const* _t = nullptr);

    // Evaluates expression, taking into account if it is a wrapped type. The
    // arguments are forwarded to ExpressionConverter. If _t is set, and wraps a
    // basic type, then _expr is cast to that type. This captures the behaviour
    // of implicitly casting raw types.
    void push(
        Expression const& _expr,
        std::shared_ptr<AnalysisStack const> _stack,
        VariableScopeResolver const& _decls,
        bool _is_ref,
        Type const* _t = nullptr
    );

    // Creates a function call to the given name, using the pushed args. The
    // args are reset after initialization.
    std::shared_ptr<CFuncCall> merge_and_pop();

    CStmtPtr merge_and_pop_stmt();

private:
    std::string const M_NAME;
    bool const M_RV_IS_REF;
    CArgList m_args;
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
    CBlockList const M_STMTS;

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
    CExprPtr const M_EXPR;

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
    std::string const M_TYPE;
    std::string const M_NAME;
    bool const M_IS_PTR;
    CExprPtr const M_INIT_VAL;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * The class of if and if/else statements.
 */
class CIf : public CStmt
{
public:
    // Represents the statement if(_cond)_true_stmt[ else _false_stmt], where
    // _false_stmt is optional.
    CIf(CExprPtr _cond, CStmtPtr _true_stmt, CStmtPtr _false_stmt = nullptr);

    ~CIf() = default;

private:
    CExprPtr const M_COND;
    CStmtPtr const M_TRUE_STMT;
    CStmtPtr const M_FALSE_STMT;

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
    CStmtPtr const M_BODY;
    CExprPtr const M_COND;
    bool const M_IS_DO_WHILE;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to a for loop in C, in which all sub-statements are optional.
 */
class CForLoop : public CStmt
{
public:
    // Constructs a while loop with given condition and body. If set to run at
    // least once, a do/while loop is generated.
    CForLoop(CStmtPtr _init, CExprPtr _cond, CStmtPtr _loop, CStmtPtr _body);

    ~CForLoop() = default;

private:
    CStmtPtr const M_INIT;
    CExprPtr const M_COND;
    CStmtPtr const M_LOOP;
    CStmtPtr const M_BODY;

    void print_impl(std::ostream & _out) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Corresponds to an integral switch statement in C. Each case is scoped and a
 * default case is required.
 */
class CSwitch : public CStmt
{
public:
    // Constructs a while loop with given condition and body. If set to run at
    // least once, a do/while loop is generated.
    CSwitch(CExprPtr _cond);
    CSwitch(CExprPtr _cond, CBlockList _default);

    // Adds a case `case _val: _body`.
    void add_case(int64_t _val, CBlockList _body);

    // Returns the number of cases, excluding default.
    size_t size() const;

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
 * A template for a return call, with or without return value.
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
 * A template for the definition of a standard C function. The function may be
 * made a forward declaration by providing no body.
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
    std::shared_ptr<CVarDecl> const M_ID;
    CParams const M_ARGS;
    std::shared_ptr<CBlock> const M_BODY;
    Modifier const M_MOD;
};

// -------------------------------------------------------------------------- //

/**
 * A template for a c-typedef. The typedef is not verified for valid types.
 */
class CTypedef : public CElement
{
public:
    // Declares type _type as _name.
    CTypedef(std::string _type, std::string _name);

    void print(std::ostream & _out) const override;

private:
    std::string const M_TYPE;
    std::string const M_NAME;
};

/**
 * A template for the definition of a standard c-structure. This function may
 * be made a forward declaration by providing no body.
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
    std::string M_NAME;
    std::shared_ptr<CParams> const M_FIELDS;
};

// -------------------------------------------------------------------------- //

}
}
}
