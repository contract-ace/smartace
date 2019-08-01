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
    ASTNode const& _ast, TypeConverter const& _converter, bool _fwd_dcl
): M_AST(_ast), M_CONVERTER(_converter), M_FWD_DCL(_fwd_dcl) {}

void ADTConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    M_AST.accept(*this);
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
    if (!M_FWD_DCL)
    {
        fields = make_shared<CParams>();
        for (auto decl : _node.stateVariables())
        {
            auto const TYPE = M_CONVERTER.get_type(*decl);
            fields->push_back(make_shared<CVarDecl>(TYPE, "d_" + decl->name()));
        }
    }
    CStructDef contract(M_CONVERTER.get_name(_node), move(fields));
    (*m_ostream) << contract;
}

void ADTConverter::endVisit(Mapping const& _node)
{
    shared_ptr<CParams> fields;
    if (!M_FWD_DCL)
    {
        string const SET_TYPE = TypeConverter::get_simple_ctype(BoolType{});
        string const KEY_TYPE = M_CONVERTER.get_type(_node.keyType());
        string const VAL_TYPE = M_CONVERTER.get_type(_node.valueType());
        fields = make_shared<CParams>(CParams{
            make_shared<CVarDecl>(SET_TYPE, "m_set"),
            make_shared<CVarDecl>(KEY_TYPE, "m_curr"),
            make_shared<CVarDecl>(VAL_TYPE, "d_"),
            make_shared<CVarDecl>(VAL_TYPE, "d_nd")
        });
    }
    CStructDef mapping(M_CONVERTER.get_name(_node), move(fields));
    (*m_ostream) << mapping;
}

void ADTConverter::endVisit(StructDefinition const& _node)
{
    shared_ptr<CParams> fields;
    if (!M_FWD_DCL)
    {
        fields = make_shared<CParams>();
        for (auto decl : _node.members())
        {
            auto const TYPE = M_CONVERTER.get_type(*decl);
            fields->push_back(make_shared<CVarDecl>(TYPE, "d_" + decl->name()));
        }
    }
    CStructDef structure(M_CONVERTER.get_name(_node), move(fields));
    (*m_ostream) << structure;
}

// -------------------------------------------------------------------------- //

}
}
}
