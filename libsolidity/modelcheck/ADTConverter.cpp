/**
 * @date 2019
 * First-pass visitor for converting Solidity AST's to models in C.
 */

#include <libsolidity/modelcheck/ADTConverter.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>
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
    for (auto structure : _node.definedStructs()) structure->accept(*this);
    for (auto decl : _node.stateVariables()) decl->accept(*this);
    return false;
}

// -------------------------------------------------------------------------- //

void ADTConverter::endVisit(ContractDefinition const& _node)
{
    shared_ptr<CParams> fields;
    if (!m_forward_declare)
    {
        fields = make_shared<CParams>();
        for (auto decl : _node.stateVariables())
        {
            auto type = m_converter.translate(*decl).type;
            fields->push_back(make_shared<CVarDecl>(type, "d_" + decl->name()));
        }
    }
    CStructDef contract(m_converter.translate(_node).name, move(fields));
    (*m_ostream) << contract;
}

void ADTConverter::endVisit(Mapping const& _node)
{
    shared_ptr<CParams> fields;
    if (!m_forward_declare)
    {
        string key_type = m_converter.translate(_node.keyType()).type;
        string val_type = m_converter.translate(_node.valueType()).type; 
        fields = make_shared<CParams>(CParams{
            make_shared<CVarDecl>("int", "m_set"),
            make_shared<CVarDecl>(key_type, "m_curr"),
            make_shared<CVarDecl>(val_type, "d_"),
            make_shared<CVarDecl>(val_type, "d_nd")
        });
    }
    CStructDef mapping(m_converter.translate(_node).name, move(fields));
    (*m_ostream) << mapping;
}

void ADTConverter::endVisit(StructDefinition const& _node)
{
    shared_ptr<CParams> fields;
    if (!m_forward_declare)
    {
        fields = make_shared<CParams>();
        for (auto decl : _node.members())
        {
            auto type = m_converter.translate(*decl).type;
            fields->push_back(make_shared<CVarDecl>(type, "d_" + decl->name()));
        }
    }
    CStructDef structure(m_converter.translate(_node).name, move(fields));
    (*m_ostream) << structure;
}

// -------------------------------------------------------------------------- //

}
}
}
