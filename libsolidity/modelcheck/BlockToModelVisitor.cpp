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
    m_decls.enter();
    m_body->accept(*this);
    m_decls.exit();
    m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool BlockToModelVisitor::visit(Block const& _node)
{
    for (auto const& stmt : _node.statements())
    {
        stmt->accept(*this);
        (*m_ostream) << endl;
    }

    return false;
}

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
    (*m_ostream) << "continue";
    end_statement();
    return false;
}

bool BlockToModelVisitor::visit(Break const&)
{
    (*m_ostream) << "break";
    end_statement();
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
    end_statement();
    return false;
}

bool BlockToModelVisitor::visit(VariableDeclarationStatement const& _node)
{
    if (_node.declarations().size() > 1)
    {
        throw runtime_error("Multiple return values are unsupported.");
    }
    else if (!_node.declarations().empty())
    {
        const auto &decl = *_node.declarations()[0];
        m_decls.record_declaration(decl);

        (*m_ostream) << m_scope.translate(decl.type()).type
                     << " "
                     << decl.name();

        if (_node.initialValue())
        {
            (*m_ostream) << " = ";
            _node.initialValue()->accept(*this);
        }

        end_statement();
    }

    return false;
}

bool BlockToModelVisitor::visit(ExpressionStatement const& _node)
{
    _node.expression().accept(*this);
    end_statement();
    return false;
}

bool BlockToModelVisitor::visit(UnaryOperation const& _node)
{
    if (!_node.isPrefixOperation())
    {
        print_subexpression(_node.subExpression());
    }

    Token op_tok = _node.getOperator();
    switch (op_tok)
    {
    case Token::Not:
	case Token::BitNot:
        (*m_ostream) << "!";
        break;
	case Token::Delete:
        // TODO(scottwe)
        throw runtime_error("Delete not yet supported.");
	case Token::Inc:
	case Token::Dec:
        (*m_ostream) << TokenTraits::friendlyName(op_tok);
        break;
    default:
        throw runtime_error("UnaryOp not yet supported.");
    }

    if (_node.isPrefixOperation())
    {
        print_subexpression(_node.subExpression());
    }

    return false;
}

bool BlockToModelVisitor::visit(BinaryOperation const& _node)
{
    print_subexpression(_node.leftExpression());

    Token op_tok = _node.getOperator();
    switch (op_tok)
    {
    case Token::SAR:
        // TODO(scottwe)
        throw runtime_error("Arithmetic right bit-shift not yet supported.");
    case Token::SHR:
        // TODO(scottwe)
        throw runtime_error("Logical right bit-shift not yet supported.");
    case Token::Exp:
        // TODO(scottwe)
        throw runtime_error("Exponentiation not yet supported.");
    case Token::Comma:
    case Token::Or:
    case Token::And:
    case Token::BitOr:
    case Token::BitXor:
    case Token::BitAnd:
    case Token::SHL:
    case Token::Add:
    case Token::Sub:
    case Token::Mul:
    case Token::Div:
    case Token::Mod:
    case Token::Equal:
    case Token::NotEqual:
    case Token::LessThan:
    case Token::GreaterThan:
    case Token::LessThanOrEqual:
    case Token::GreaterThanOrEqual:
        (*m_ostream) << TokenTraits::friendlyName(op_tok);
        break;
    default:
        throw runtime_error("BinOp not yet supported.");
    }

    print_subexpression(_node.rightExpression());

    return false;
}

bool BlockToModelVisitor::visit(Identifier const& _node)
{
    (*m_ostream) << m_decls.resolve_identifier(_node);
    return false;
}

bool BlockToModelVisitor::visit(IfStatement const& _node)
{
    (*m_ostream) << "if (";
    _node.condition().accept(*this);
    (*m_ostream) << ") {" << endl;
    m_decls.enter();
    _node.trueStatement().accept(*this);
    m_decls.exit();
    (*m_ostream) << "}";
    if (_node.falseStatement())
    {
        (*m_ostream) << " else {" << endl;
        m_decls.enter();
        _node.falseStatement()->accept(*this);
        m_decls.exit();
        (*m_ostream) << "}";
    }
    return false;
}
bool BlockToModelVisitor::visit(WhileStatement const& _node)
{
    // TODO(scottwe): Ensure finite execution.

    if (_node.isDoWhile())
    {
        (*m_ostream) << "do {" << endl;
        _node.body().accept(*this);
        (*m_ostream) << "} while (";
        print_loop_statement(&_node.condition());
        (*m_ostream) << ")";
        end_statement();
    }
    else
    {
        (*m_ostream) << "while (";
        print_loop_statement(&_node.condition());
        (*m_ostream) << ") {" << endl;
        _node.body().accept(*this);
        (*m_ostream) << "}";
    }

    return false;
}

bool BlockToModelVisitor::visit(ForStatement const& _node)
{
    // TODO(scottwe): Ensure finite execution.

    (*m_ostream) << "for (";
    print_loop_statement(_node.initializationExpression());
    (*m_ostream) << "; ";
    print_loop_statement(_node.condition());
    (*m_ostream) << "; ";
    print_loop_statement(_node.loopExpression());
    (*m_ostream) << ") {" << endl;
    _node.body().accept(*this);
    (*m_ostream) << "}";

    return false;
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

void BlockToModelVisitor::print_subexpression(Expression const& _node)
{
    (*m_ostream) << "(";
    _node.accept(*this);
    (*m_ostream) << ")";
}

void BlockToModelVisitor::print_loop_statement(ASTNode const* _node)
{
    if (_node)
    {
        m_is_loop_statement = true;
        _node->accept(*this);
        m_is_loop_statement = false;
    }
}

void BlockToModelVisitor::end_statement()
{
    if (!m_is_loop_statement)
    {
        (*m_ostream) << ";";
    }
}

// -------------------------------------------------------------------------- //

}
}
}
