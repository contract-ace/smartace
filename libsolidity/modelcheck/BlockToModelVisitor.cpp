/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#include <libsolidity/modelcheck/BlockToModelVisitor.h>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

BlockToModelVisitor::BlockToModelVisitor(
    Block const& _body,
    TypeTranslator const& _scope
): m_body(&_body), m_scope(_scope)
{
}

void BlockToModelVisitor::print(ostream& _stream)
{
    m_ostream = &_stream;
    m_body->accept(*this);
    m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool BlockToModelVisitor::visit(Literal const& _node)
{
    switch (_node.token())
    {
	case Token::TrueLiteral:
        (*m_ostream) << "1";
        break;
	case Token::FalseLiteral:
        (*m_ostream) << "0";
		break;
	case Token::Number:
        (*m_ostream) << stoll(_node.value());
		break;
	case Token::StringLiteral:
        (*m_ostream) << m_hasher(_node.value());
		break;
    default:
        throw runtime_error("Literal type derived from unsupported token.");
    }
    return false;
}

bool BlockToModelVisitor::visit(ExpressionStatement const& _node)
{
    _node.expression().accept(*this);
    (*m_ostream) << ";" << endl;
    return false;
}

bool BlockToModelVisitor::visit(IfStatement const& _node)
{
    (*m_ostream) << "if (";
    _node.condition().accept(*this);
    (*m_ostream) << ") {" << endl;
    _node.trueStatement().accept(*this);
    (*m_ostream) << "}";
    if (_node.falseStatement())
    {
        (*m_ostream) << " else {" << endl;
        _node.falseStatement()->accept(*this);
        (*m_ostream) << "}";
    }
    (*m_ostream) << endl;
    return false;
}

// -------------------------------------------------------------------------- //

}
}
}
