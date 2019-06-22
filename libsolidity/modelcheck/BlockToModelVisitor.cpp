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

long long int BlockToModelVisitor::literal_to_number(Literal const& _node)
{
    long long int num;

    istringstream iss(_node.value());
    iss >> num;

    switch(_node.subDenomination())
    {
    case Literal::SubDenomination::Szabo:
        num *= 1000000000000;
        break;
    case Literal::SubDenomination::Finney:
        num *= 1000000000000000;
        break;
    case Literal::SubDenomination::Ether:
        num *= 1000000000000000000;
        break;
    case Literal::SubDenomination::Minute:
        num *= 60;
        break;
    case Literal::SubDenomination::Hour:
        num *= 60 * 60;
        break;
    case Literal::SubDenomination::Day:
        num *= 60 * 60 * 24;
        break;
    case Literal::SubDenomination::Week:
        num *= 60 * 60 * 24 * 7;
        break;
    case Literal::SubDenomination::Year:
        num *= 60 * 60 * 24 * 365;
        break;
    default:
        break;
    }

    return num;
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
        (*m_ostream) << literal_to_number(_node);
		break;
	case Token::StringLiteral:
        (*m_ostream) << m_hasher(_node.value());
		break;
    default:
        throw runtime_error("Literal type derived from unsupported token.");
    }
    return false;
}

bool BlockToModelVisitor::visit(Continue const&)
{
    (*m_ostream) << "continue;" << endl;
    return false;
}

bool BlockToModelVisitor::visit(Break const&)
{
    (*m_ostream) << "break;" << endl;
    return false;
}

bool BlockToModelVisitor::visit(Return const& _node)
{
    (*m_ostream) << "return";
    if (_node.expression())
    {
        (*m_ostream) << " ";
        _node.expression()->accept(*this);
    }
    (*m_ostream) << ";" << endl;
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
