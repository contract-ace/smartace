/**
 * @date 2019
 * First-pass visitor for converting Solidity AST's to models in C.
 */

#include <libsolidity/modelcheck/ADTConverter.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ADTConverter::ADTConverter(
    ASTNode const& _ast,
    TypeConverter const& _converter,
    bool _forward_declare
): m_ast(&_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void ADTConverter::print(ostream& _stream)
{
    m_ostream = &_stream;
    m_ast->accept(*this);
    m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool ADTConverter::visit(ContractDefinition const& _node)
{
    (*m_ostream) << m_converter.translate(_node).type;
    if (!m_forward_declare)
    {
        (*m_ostream) << endl << "{" << endl;
        for (auto decl : _node.stateVariables())
        {
            auto type = m_converter.translate(*decl).type;
            (*m_ostream) << type << " d_" << decl->name() << ";" << endl;
        }
        (*m_ostream) << "}";
    }
    (*m_ostream) << ";" << endl;
    return true;
}

bool ADTConverter::visit(Mapping const& _node)
{
    (*m_ostream) << m_converter.translate(_node).type;
    if (!m_forward_declare)
    {
        string key_type = m_converter.translate(_node.keyType()).type;
        string val_type = m_converter.translate(_node.valueType()).type;

        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << "int m_set;" << endl;
        (*m_ostream) << key_type << " m_curr;" << endl;
        (*m_ostream) << val_type << " d_;" << endl;
        (*m_ostream) << "}";
    }
    (*m_ostream) << ";" << endl;
    return true;
}

bool ADTConverter::visit(StructDefinition const& _node)
{
    (*m_ostream) << m_converter.translate(_node).type;
    if (!m_forward_declare)
    {
        (*m_ostream) << endl << "{" << endl;
        for (auto decl : _node.members())
        {
            auto type = m_converter.translate(*decl).type;
            (*m_ostream) << type << " d_" << decl->name() << ";" << endl;
        }
        (*m_ostream) << "}";
    }
    (*m_ostream) << ";" << endl;
    return true;
}

// -------------------------------------------------------------------------- //

bool ADTConverter::visit(EventDefinition const&)
{
    return false;
}

bool ADTConverter::visit(FunctionDefinition const&)
{
    return false;
}

bool ADTConverter::visit(ModifierDefinition const&)
{
    return false;
}

// -------------------------------------------------------------------------- //

}
}
}
