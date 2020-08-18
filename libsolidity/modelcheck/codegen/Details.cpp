#include <libsolidity/modelcheck/codegen/Details.h>

#include <libsolidity/modelcheck/model/Expression.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Types.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

shared_ptr<CAssign> CData::assign(CExprPtr _rhs) const
{
    return make_shared<CAssign>(expr(), move(_rhs));
}

shared_ptr<CMemberAccess> CData::access(string _member) const
{
    return make_shared<CMemberAccess>(expr(), move(_member));
}

// -------------------------------------------------------------------------- //

CComment::CComment(string _val) : m_val(move(_val))
{
    nest();
}

void CComment::print_impl(ostream & _out) const
{
    _out << "/* " << m_val << " */" << endl;
}

// -------------------------------------------------------------------------- //

CIdentifier::CIdentifier(string _name, bool _ptr)
: M_NAME(move(_name)), M_IS_PTR(_ptr) {}

void CIdentifier::print(ostream & _out) const { _out << M_NAME; }

bool CIdentifier::is_pointer() const { return M_IS_PTR; }

CExprPtr CIdentifier::expr() const
{
    return make_shared<CIdentifier>(M_NAME, M_IS_PTR);
}

// -------------------------------------------------------------------------- //

CIntLiteral::CIntLiteral(long long int _val): M_VAL(_val) {}

void CIntLiteral::print(ostream & _out) const { _out << M_VAL; }

// -------------------------------------------------------------------------- //

string CStringLiteral::escape_cstring(string _val)
{
    ostringstream stream;
    stream << "\"";
    for (char c : _val)
    {
        if (c == '\"') stream << "\\\"";
        else stream << c;
    }
    stream << "\"";
    return stream.str();
}

CStringLiteral::CStringLiteral(string const& _val)
 : M_VAL(escape_cstring(move(_val)))
{
}

void CStringLiteral::print(ostream & _out) const { _out << M_VAL; }

// -------------------------------------------------------------------------- //

CUnaryOp::CUnaryOp(string _op, CExprPtr _expr, bool _pre)
: M_OP(move(_op)), M_EXPR(move(_expr)), M_PRE(_pre) {}

void CUnaryOp::print(ostream & _out) const
{
    if (M_PRE) _out << M_OP;
    _out << "(" << *M_EXPR << ")";
    if (!M_PRE) _out << M_OP;
}

CStmtPtr CUnaryOp::stmt()
{
    return make_shared<CExprStmt>(make_shared<CUnaryOp>(M_OP, M_EXPR, M_PRE));
}

CReference::CReference(CExprPtr _expr): CUnaryOp("&", move(_expr), true) {}

bool CReference::is_pointer() const { return true; }

CDereference::CDereference(CExprPtr _expr): CUnaryOp("*", move(_expr), true) {}

// -------------------------------------------------------------------------- //

CBinaryOp::CBinaryOp(CExprPtr _lhs, string _op, CExprPtr _rhs)
: M_LHS(move(_lhs)), M_RHS(move(_rhs)), M_OP(move(_op)) {}

CStmtPtr CBinaryOp::stmt()
{
    return make_shared<CExprStmt>(make_shared<CBinaryOp>(M_LHS, M_OP, M_RHS));
}

void CBinaryOp::print(ostream & _out) const
{
    _out << "(" << *M_LHS << ")" << M_OP << "(" << *M_RHS << ")";
}

CAssign::CAssign(CExprPtr _lhs, CExprPtr _rhs): CBinaryOp(_lhs, "=", _rhs) {}

// -------------------------------------------------------------------------- //

CCond::CCond(CExprPtr _cond, CExprPtr _true_case, CExprPtr _false_case)
 : M_COND(move(_cond))
 , M_TRUE_CASE(move(_true_case))
 , M_FALSE_CASE(move(_false_case))
{}

void CCond::print(ostream & _out) const
{
    _out << "(" << *M_COND << ")";
    _out << "?";
    _out << "(" << *M_TRUE_CASE << ")";
    _out << ":";
    _out << "(" << *M_FALSE_CASE << ")";
}

bool CCond::is_pointer() const { return M_TRUE_CASE->is_pointer(); }

CStmtPtr CCond::stmt()
{
    auto cond = make_shared<CCond>(M_COND, M_TRUE_CASE, M_FALSE_CASE);
    return make_shared<CExprStmt>(move(cond));
}

// -------------------------------------------------------------------------- //

CMemberAccess::CMemberAccess(CExprPtr _expr, string _member)
 : M_EXPR(move(_expr)), M_MEMBER(move(_member)) {}

void CMemberAccess::print(ostream & _out) const
{
    bool is_ptr = M_EXPR->is_pointer();
    _out << "(" << *M_EXPR << ")" << (is_ptr ? "->" : ".") << M_MEMBER;
}

CExprPtr CMemberAccess::expr() const
{
    return make_shared<CMemberAccess>(M_EXPR, M_MEMBER);
}

// -------------------------------------------------------------------------- //

CCast::CCast(CExprPtr _expr, string _type)
: M_EXPR(move(_expr)), M_TYPE(move(_type)) {}

void CCast::print(ostream & _out) const
{
    _out << "((" << M_TYPE << ")(" << *M_EXPR << "))";
}

bool CCast::is_pointer() const { return M_EXPR->is_pointer(); }

// -------------------------------------------------------------------------- //

CFuncCall::CFuncCall(string _name, CArgList _args, bool _rv_is_ref)
 : M_NAME(move(_name))
 , M_RV_IS_REF(_rv_is_ref)
 , M_ARGS(move(_args))
{
}

void CFuncCall::print(ostream & _out) const
{
    _out << M_NAME << "(";
    for (auto arg = M_ARGS.cbegin(); arg != M_ARGS.cend(); ++arg)
    {
        if (arg != M_ARGS.cbegin()) _out << ",";
        _out << *(*arg);
    }
    _out << ")";
}

bool CFuncCall::is_pointer() const
{
    return M_RV_IS_REF;
}

CStmtPtr CFuncCall::stmt()
{
    return make_shared<CExprStmt>(make_shared<CFuncCall>(M_NAME, M_ARGS));
}

CFuncCallBuilder::CFuncCallBuilder(string _name, bool _rv_is_ref)
 : M_NAME(move(_name))
 , M_RV_IS_REF(_rv_is_ref)
{
}

void CFuncCallBuilder::push(CExprPtr _expr, Type const* _t)
{
    if (_t) _expr = InitFunction::wrap(*_t, move(_expr));
    m_args.push_back(move(_expr));
}

void CFuncCallBuilder::push(
    Expression const& _expr,
    shared_ptr<AnalysisStack const> _stack,
    VariableScopeResolver const& _decls,
    bool _is_ref,
    Type const* _t
)
{
    ExpressionConverter converter(_expr, _stack, _decls, _is_ref);

    if (!_t) _t = _expr.annotation().type;

    auto c_expr = converter.convert();
    if (_t) c_expr = InitFunction::wrap(*_t, move(c_expr));
    m_args.push_back(move(c_expr));
}

shared_ptr<CFuncCall> CFuncCallBuilder::merge_and_pop()
{
    return make_shared<CFuncCall>(M_NAME, move(m_args), M_RV_IS_REF);
}

CStmtPtr CFuncCallBuilder::merge_and_pop_stmt()
{
    return merge_and_pop()->stmt();
}

// -------------------------------------------------------------------------- //

CBlock::CBlock(CBlockList _stmts) : M_STMTS(move(_stmts)) { nest(); }

void CBlock::print_impl(ostream & _out) const
{
    _out << "{";
    for (auto stmt : M_STMTS) _out << *stmt;
    _out << "}";
}

// -------------------------------------------------------------------------- //

CExprStmt::CExprStmt(CExprPtr _expr): M_EXPR(move(_expr)) {}

void CExprStmt::print_impl(ostream & _out) const { _out << *M_EXPR; }

// -------------------------------------------------------------------------- //

CVarDecl::CVarDecl(string _type, string _name, bool _ptr, CExprPtr _init)
 : M_TYPE(move(_type))
 , M_NAME(move(_name))
 , M_IS_PTR(_ptr)
 , M_INIT_VAL(move(_init)) {}

CVarDecl::CVarDecl(string _type, string _name, bool _ptr)
: CVarDecl(move(_type), move(_name), _ptr, nullptr) {}

CVarDecl::CVarDecl(string _type, string _name)
: CVarDecl(move(_type), move(_name), false) {}

shared_ptr<CIdentifier> CVarDecl::id() const
{
    return make_shared<CIdentifier>(M_NAME, M_IS_PTR);
}

CExprPtr CVarDecl::expr() const
{
    return id();
}

void CVarDecl::print_impl(ostream & _out) const
{
    _out << M_TYPE << (M_IS_PTR ? "*" : " ") << M_NAME;
    if (M_INIT_VAL) _out << "=" << *M_INIT_VAL;
}

// -------------------------------------------------------------------------- //

CIf::CIf(CExprPtr _cond, CStmtPtr _true_stmt, CStmtPtr _false_stmt)
 : M_COND(move(_cond))
 , M_TRUE_STMT(move(_true_stmt))
 , M_FALSE_STMT(move(_false_stmt))
{
    nest();
}

void CIf::print_impl(ostream & _out) const
{
    _out << "if(" << *M_COND << ")" << *M_TRUE_STMT;
    if (M_FALSE_STMT) _out << "else " << *M_FALSE_STMT;
}

// -------------------------------------------------------------------------- //

CWhileLoop::CWhileLoop(CStmtPtr _body, CExprPtr _cond, bool _atleast_once)
 : M_BODY(move(_body)), M_COND(move(_cond)), M_IS_DO_WHILE(_atleast_once)
{
    if (!_atleast_once) nest();
}

void CWhileLoop::print_impl(ostream & _out) const
{
    if (M_IS_DO_WHILE)
    {
        _out << "do" << *M_BODY << "while(" << *M_COND << ")";
    }
    else
    {
        _out << "while(" << *M_COND << ")" << *M_BODY;
    }
}

// -------------------------------------------------------------------------- //

CForLoop::CForLoop(
    CStmtPtr _init, CExprPtr _cond, CStmtPtr _loop, CStmtPtr _body
): M_INIT(move(_init)),
   M_COND(move(_cond)),
   M_LOOP(move(_loop)),
   M_BODY(move(_body))
{
    nest();
    if (M_INIT) M_INIT->nest();
    if (M_LOOP) M_LOOP->nest();
}

void CForLoop::print_impl(ostream & _out) const
{
    _out << "for(";
    if (M_INIT) _out << *M_INIT;
    _out << ";";
    if (M_COND) _out << *M_COND;
    _out << ";";
    if (M_LOOP) _out << *M_LOOP;
    _out << ")" << *M_BODY;
}

// -------------------------------------------------------------------------- //

CSwitch::CSwitch(CExprPtr _cond): CSwitch(_cond, {make_shared<CBreak>()}) {}

CSwitch::CSwitch(CExprPtr _cond, CBlockList _default)
: m_cond(_cond), m_default(move(_default))
{
    m_cond.nest();
    nest();
}

void CSwitch::add_case(int64_t _val, CBlockList _body)
{
    m_cases.emplace(make_pair(_val, move(_body)));
}

size_t CSwitch::size() const { return m_cases.size(); }

void CSwitch::print_impl(ostream & _out) const
{
    _out << "switch(" << m_cond << "){";
    for (auto const switch_case : m_cases)
    {
        _out << "case " << switch_case.first << ":" << switch_case.second;
    }
    _out << "default:" << m_default << "}";
}

// -------------------------------------------------------------------------- //

void CBreak::print_impl(ostream & _out) const { _out << "break"; }

void CContinue::print_impl(ostream & _out) const { _out << "continue"; }

// -------------------------------------------------------------------------- //

CReturn::CReturn(CExprPtr _retval): m_retval(move(_retval)) {}

void CReturn::print_impl(ostream & _out) const
{
    _out << "return";
    if (m_retval) _out << " " << *m_retval;
}

// -------------------------------------------------------------------------- //

CFuncDef::CFuncDef(
    shared_ptr<CVarDecl> _id,
    CParams _args,
    shared_ptr<CBlock> _body,
    CFuncDef::Modifier _mod
): M_ID(move(_id)), M_ARGS(move(_args)), M_BODY(move(_body)), M_MOD(_mod)
{
    M_ID->nest();
    for (auto arg : M_ARGS) arg->nest();
}

void CFuncDef::print(ostream & _out) const
{
    if (M_MOD == Modifier::INLINE)
    {
        _out << "static inline ";
    }
    else if (M_MOD == Modifier::EXTERN)
    {
        _out << "extern ";
    }

    _out << *M_ID << "(";
    if (M_ARGS.empty())
    {
        _out << "void";
    }
    else
    {
        for (auto itr = M_ARGS.begin(); itr != M_ARGS.end(); ++itr)
        {
            if (itr != M_ARGS.begin()) _out << ",";
            _out << *(*itr);
        }
    }
    _out << ")";

    if (M_BODY)
    {
        _out << *M_BODY;
    }
    else
    {
        _out << ";";
    }
}

// -------------------------------------------------------------------------- //

CTypedef::CTypedef(string _type, string _name)
 : M_TYPE(move(_type)), M_NAME(move(_name)) {}

void CTypedef::print(ostream & _out) const
{
    _out << "typedef " << M_TYPE << " " << M_NAME << ";";
}

CStructDef::CStructDef(string _name, shared_ptr<CParams> _fields)
 : M_NAME(move(_name)), M_FIELDS(move(_fields)) {}

void CStructDef::print(ostream & _out) const
{
    _out << "struct " << M_NAME;
    if (M_FIELDS)
    {
        _out << "{";
        for (auto field : *M_FIELDS) _out << *field;
        _out << "}";
    }
    _out << ";";
}

shared_ptr<CTypedef> CStructDef::make_typedef(string _name)
{
    return make_shared<CTypedef>("struct " + M_NAME, _name);
}

shared_ptr<CVarDecl> CStructDef::decl(string _name)
{
    return decl(_name, false);
}

shared_ptr<CVarDecl> CStructDef::decl(string _name, bool _ptr)
{
    return make_shared<CVarDecl>("struct " + M_NAME, _name, _ptr);
}

// -------------------------------------------------------------------------- //

}
}
}
