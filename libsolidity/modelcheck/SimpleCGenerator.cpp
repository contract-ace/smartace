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

CMemberAccess::CMemberAccess(CExprPtr const _expr, string _member)
: m_expr(move(_expr)), m_member(move(_member)) {}

void CMemberAccess::print(ostream & _out) const
{
    bool is_ptr = m_expr->is_pointer();
    _out << "(" << *m_expr << ")" << (is_ptr ? "->" : ".") << m_member;
}

// -------------------------------------------------------------------------- //

CCast::CCast(CExprPtr const _expr, string _type)
: m_expr(move(_expr)), m_type(move(_type)) {}

void CCast::print(std::ostream & _out) const
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

}
}
}
