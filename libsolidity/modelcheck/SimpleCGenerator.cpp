/**
 * @date 2019
 * A limited, AST-like library specialized for transpiling Solidity into C.
 */

#include <libsolidity/modelcheck/SimpleCGenerator.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CIdentifier::CIdentifier(string _name, bool _ptr)
: m_name(move(_name)), m_ptr(_ptr) {}

void CIdentifier::print(ostream & _out) const
{
    _out << m_name;
}

bool CIdentifier::is_pointer() const
{
    return m_ptr;
}

// -------------------------------------------------------------------------- //

CIntLiteral::CIntLiteral(long long int _val): m_val(_val) {}

void CIntLiteral::print(ostream & _out) const
{
    _out << m_val;
}

// -------------------------------------------------------------------------- //

CUnaryOp::CUnaryOp(string _op, CExprPtr _expr, bool _pre)
: m_op(move(_op)), m_expr(move(_expr)), m_pre(_pre) {}

void CUnaryOp::print(ostream & _out) const
{
    if (m_pre) _out << m_op;
    _out << "(" << *m_expr << ")";
    if (!m_pre) _out << m_op;
}

CReference::CReference(CExprPtr _expr): CUnaryOp("&", move(_expr), true) {}

bool CReference::is_pointer() const
{
    return true;
}

CDereference::CDereference(CExprPtr _expr): CUnaryOp("*", move(_expr), true) {}

// -------------------------------------------------------------------------- //

CBinaryOp::CBinaryOp(CExprPtr _lhs, string _op, CExprPtr _rhs)
: m_lhs(move(_lhs)), m_rhs(move(_rhs)), m_op(move(_op)) {}

void CBinaryOp::print(ostream & _out) const
{
    _out << "(" << *m_lhs << ")" << m_op << "(" << *m_rhs << ")";
}

CAssign::CAssign(CExprPtr _lhs, CExprPtr _rhs): CBinaryOp(_lhs, "=", _rhs) {}

// -------------------------------------------------------------------------- //

CCond::CCond(CExprPtr _cond, CExprPtr _tcase, CExprPtr _fcase)
: m_cond(move(_cond)), m_tcase(move(_tcase)), m_fcase(move(_fcase)) {}

void CCond::print(ostream & _out) const
{
    _out << "(" << *m_cond << ")?(" << *m_tcase << "):(" << *m_fcase << ")";
}

bool CCond::is_pointer() const
{
    return m_tcase->is_pointer();
}

// -------------------------------------------------------------------------- //

CMemberAccess::CMemberAccess(CExprPtr _expr, string _member)
: m_expr(move(_expr)), m_member(move(_member)) {}

void CMemberAccess::print(ostream & _out) const
{
    bool is_ptr = m_expr->is_pointer();
    _out << "(" << *m_expr << ")" << (is_ptr ? "->" : ".") << m_member;
}

// -------------------------------------------------------------------------- //

CCast::CCast(CExprPtr _expr, string _type)
: m_expr(move(_expr)), m_type(move(_type)) {}

void CCast::print(ostream & _out) const
{
    _out << "((" << m_type << ")(" << *m_expr << "))";
}

bool CCast::is_pointer() const
{
    return m_expr->is_pointer();
}

// -------------------------------------------------------------------------- //

CFuncCall::CFuncCall(string _name, CArgList _args)
: m_name(move(_name)), m_args(move(_args)) {}

void CFuncCall::print(ostream & _out) const
{
    _out << m_name << "(";
    for (auto arg = m_args.cbegin(); arg != m_args.cend(); ++arg)
    {
        if (arg != m_args.cbegin()) _out << ",";
        _out << *(*arg);
    }
    _out << ")";
}

// -------------------------------------------------------------------------- //

CBlock::CBlock(CBlockList _stmts) : m_stmts(move(_stmts))
{
    nest();
}

void CBlock::print_impl(std::ostream & _out) const
{
    _out << "{";
    for (auto stmt : m_stmts) _out << *stmt;
    _out << "}";
}

// -------------------------------------------------------------------------- //

CExprStmt::CExprStmt(CExprPtr _expr): m_expr(move(_expr)) {}

void CExprStmt::print_impl(ostream & _out) const
{
    _out << *m_expr;
}

// -------------------------------------------------------------------------- //

CVarDecl::CVarDecl(string _type, string _name, bool _ptr, CExprPtr _init)
: m_type(move(_type)), m_name(move(_name)), m_ptr(_ptr), m_init(move(_init)) {}

CVarDecl::CVarDecl(string _type, string _name, bool _ptr)
: CVarDecl(move(_type), move(_name), _ptr, nullptr) {}

CVarDecl::CVarDecl(string _type, string _name)
: CVarDecl(move(_type), move(_name), false) {}

shared_ptr<CIdentifier> CVarDecl::id() const
{
    return make_shared<CIdentifier>(m_name, m_ptr);
}

void CVarDecl::print_impl(ostream & _out) const
{
    _out << m_type << (m_ptr ? "*" : " ") << m_name;
    if (m_init) _out << "=" << *m_init;
}

// -------------------------------------------------------------------------- //

CIf::CIf(CExprPtr _cond, CStmtPtr _tstmt, CStmtPtr _fstmt)
: m_cond(move(_cond)), m_tstmt(move(_tstmt)), m_fstmt(move(_fstmt))
{
    nest();
}

void CIf::print_impl(ostream & _out) const
{
    _out << "if(" << *m_cond << ")" << *m_tstmt;
    if (m_fstmt) _out << "else " << *m_fstmt;
}

// -------------------------------------------------------------------------- //

CWhileLoop::CWhileLoop(CStmtPtr _body, CExprPtr _cond, bool _atleast_once)
: m_body(move(_body)), m_cond(move(_cond)), m_dowhile(_atleast_once)
{
    if (!_atleast_once) nest();
}

void CWhileLoop::print_impl(ostream & _out) const
{
    if (m_dowhile)
    {
        _out << "do" << *m_body << "while(" << *m_cond << ")";
    }
    else
    {
        _out << "while(" << *m_cond << ")" << *m_body;
    }
}

// -------------------------------------------------------------------------- //

CForLoop::CForLoop(
    CStmtPtr _init, CExprPtr _cond, CStmtPtr _loop, CStmtPtr _body
): m_init(move(_init)),
   m_cond(move(_cond)),
   m_loop(move(_loop)),
   m_body(move(_body))
{
    nest();
    if (m_init) m_init->nest();
    if (m_loop) m_loop->nest();
}

void CForLoop::print_impl(ostream & _out) const
{
    _out << "for(";
    if (m_init) _out << *m_init;
    _out << ";";
    if (m_cond) _out << *m_cond;
    _out << ";";
    if (m_loop) _out << *m_loop;
    _out << ")" << *m_body;
}

// -------------------------------------------------------------------------- //

void CBreak::print_impl(ostream & _out) const
{
    _out << "break";
}

void CContinue::print_impl(ostream & _out) const
{
    _out << "continue";
}

// -------------------------------------------------------------------------- //

CReturn::CReturn(CExprPtr _retval): m_retval(move(_retval)) {}

void CReturn::print_impl(ostream & _out) const
{
    _out << "return";
    if (m_retval) _out << " " << *m_retval;
}

// -------------------------------------------------------------------------- //

CFuncDef::CFuncDef(
    shared_ptr<CVarDecl> _id, CParams _args, shared_ptr<CBlock> _body
): m_id(move(_id)), m_args(move(_args)), m_body(move(_body))
{
    m_id->nest();
    for (auto arg : m_args) arg->nest();
}

void CFuncDef::print(ostream & _out) const
{
    _out << *m_id << "(";
    for (auto itr = m_args.begin(); itr != m_args.end(); ++itr)
    {
        if (itr != m_args.begin()) _out << ",";
        _out << *(*itr);
    }
    _out << ")";
    if (m_body)
    {
        _out << *m_body;
    }
    else
    {
        _out << ";";
    }
}

// -------------------------------------------------------------------------- //

}
}
}
