/**
 * @date 2019
 * First-pass visitor for converting Solidity AST's to models in C.
 */

#include <libsolidity/modelcheck/ADTConverter.h>

#include <libsolidity/modelcheck/Utility.h>
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
): m_ast(_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void ADTConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}

// -------------------------------------------------------------------------- //

bool ADTConverter::visit(ContractDefinition const& _node)
{
    for (auto structure : _node.definedStructs())
    {
        structure->accept(*this);
    }

    for (auto decl : _node.stateVariables())
    {
        decl->accept(*this);
    }

    return false;
}

// -------------------------------------------------------------------------- //

void ADTConverter::endVisit(ContractDefinition const& _node)
{
    (*m_ostream) << m_converter.translate(_node).type;
    if (!m_forward_declare)
    {
        (*m_ostream) << "{";
        for (auto decl : _node.stateVariables())
        {
            auto type = m_converter.translate(*decl).type;
            (*m_ostream) << type << " d_" << decl->name() << ";";
        }
        (*m_ostream) << "}";
    }
    (*m_ostream) << ";";
}

void ADTConverter::endVisit(Mapping const& _node)
{
    (*m_ostream) << m_converter.translate(_node).type;
    if (!m_forward_declare)
    {
        string key_type = m_converter.translate(_node.keyType()).type;
        string val_type = m_converter.translate(_node.valueType()).type;

        (*m_ostream) << "{";
        (*m_ostream) << "int m_set;";
        (*m_ostream) << key_type << " m_curr;";
        (*m_ostream) << val_type << " d_;";
        (*m_ostream) << val_type << " d_nd;";
        (*m_ostream) << "}";
    }
    (*m_ostream) << ";";
}

void ADTConverter::endVisit(StructDefinition const& _node)
{
    (*m_ostream) << m_converter.translate(_node).type;
    if (!m_forward_declare)
    {
        (*m_ostream) << "{";
        for (auto decl : _node.members())
        {
            auto type = m_converter.translate(*decl).type;
            (*m_ostream) << type << " d_" << decl->name() << ";";
        }
        (*m_ostream) << "}";
    }
    (*m_ostream) << ";";
}

// -------------------------------------------------------------------------- //

}
}
}
