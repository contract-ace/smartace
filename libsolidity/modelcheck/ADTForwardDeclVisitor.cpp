/**
 * @date 2019
 * First-pass visitor for converting Solidity AST's to models in C.
 */

#include <libsolidity/modelcheck/ADTForwardDeclVisitor.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ADTForwardDeclVisitor::ADTForwardDeclVisitor(
    ASTNode const& _ast
): m_ast(&_ast)
{
}

void ADTForwardDeclVisitor::print(ostream& _stream)
{
    m_ostream = &_stream;
    m_ast->accept(*this);
    m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool ADTForwardDeclVisitor::visit(ContractDefinition const& _node)
{
    m_translator.enter_scope(_node);
    (*m_ostream) << m_translator.translate(_node).type << ";" << endl;
    return true;
}

bool ADTForwardDeclVisitor::visit(Mapping const& _node)
{
    (*m_ostream) << m_translator.translate(_node).type << ";" << endl;
    return true;
}

bool ADTForwardDeclVisitor::visit(VariableDeclaration const& _node)
{
    m_translator.enter_scope(_node);
    return true;
}

bool ADTForwardDeclVisitor::visit(StructDefinition const& _node)
{
    m_translator.enter_scope(_node);
    (*m_ostream) << m_translator.translate(_node).type << ";" << endl;
    return true;
}

// -------------------------------------------------------------------------- //

bool ADTForwardDeclVisitor::visit(EventDefinition const&)
{
    return false;
}

bool ADTForwardDeclVisitor::visit(FunctionDefinition const&)
{
    return false;
}

bool ADTForwardDeclVisitor::visit(ModifierDefinition const&)
{
    return false;
}

// -------------------------------------------------------------------------- //

void ADTForwardDeclVisitor::endVisit(ContractDefinition const&)
{
    m_translator.exit_scope();
}

void ADTForwardDeclVisitor::endVisit(VariableDeclaration const&)
{
    m_translator.exit_scope();
}

void ADTForwardDeclVisitor::endVisit(StructDefinition const&)
{
    m_translator.exit_scope();
}

// -------------------------------------------------------------------------- //

}
}
}
