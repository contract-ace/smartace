/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/FunctionDefinitionGenerator.h>
#include <libsolidity/modelcheck/FunctionForwardDeclVisitor.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

FunctionForwardDeclVisitor::FunctionForwardDeclVisitor(
    ASTNode const& _ast
): m_ast(&_ast)
{
}

void FunctionForwardDeclVisitor::print(ostream& _stream)
{
    m_ostream = &_stream;
    m_ast->accept(*this);
    m_ostream = nullptr;
}

// -------------------------------------------------------------------------- //

bool FunctionForwardDeclVisitor::visit(ContractDefinition const& _node)
{
    m_translator.enter_scope(_node);
    if (_node.constructor() == nullptr)
    {
        FunctionDefinitionGenerator contract_ctor_gen(_node, true);
        // TODO: populate body
        auto contract_ctor_node = contract_ctor_gen.generate();
        contract_ctor_node->accept(*this);
    }
    return true;
}

bool FunctionForwardDeclVisitor::visit(StructDefinition const& _node)
{
    m_translator.enter_scope(_node);
    // Generates the AST for for a struct's default constructor.
    FunctionDefinitionGenerator struct_ctor_gen(_node, true);
    for (const auto &param : _node.members())
    {
        const auto &type = param->type();
        if (!dynamic_cast<const ArrayType *>(type) &&
            !dynamic_cast<const MappingType *>(type))
        {
            struct_ctor_gen.push_arg(param);
        }
    }
    // TODO: populate body
    auto struct_ctor_node = struct_ctor_gen.generate();
    struct_ctor_node->accept(*this);
    return true;
}

bool FunctionForwardDeclVisitor::visit(FunctionDefinition const& _node)
{
    if (_node.isConstructor())
    {
        (*m_ostream) << m_translator.scope().type;
    }
    else
    {
        printRetvals(_node);
    }

    (*m_ostream) << " ";

    (*m_ostream) << ((_node.isConstructor()) ? "Ctor" : "Method")
                 << "_"
                 << m_translator.scope().name;
    if (!_node.isConstructor())
    {
        (*m_ostream) << "_" << _node.name();
    }

    printArgs(_node);

    (*m_ostream) << ";" << endl;

    return false;
}

bool FunctionForwardDeclVisitor::visit(ModifierDefinition const& _node)
{
    (*m_ostream) << "void"
                 << " "
                 << "Modifier"
                 << "_"
                 << m_translator.scope().name
                 << "_"
                 << _node.name();

    printArgs(_node);

    (*m_ostream) << ";" << endl;

    return false;
}

bool FunctionForwardDeclVisitor::visit(VariableDeclaration const& _node)
{
    m_translator.enter_scope(_node);
    return true;
}

bool FunctionForwardDeclVisitor::visit(Mapping const& _node)
{
    Translation map_translation = m_translator.translate(_node);

    string key_type = m_translator.translate(_node.keyType()).type;
    string val_type = m_translator.translate(_node.valueType()).type;

    (*m_ostream) << val_type << " "
                 << "Read" << "_" << map_translation.name
                 << "(" << map_translation.type << " *a, "
                        << key_type << " idx"
                 << ");"
                 << endl;

    (*m_ostream) << "void "
                 << "Write" << "_" << map_translation.name
                 << "(" << map_translation.type << " *a, "
                        << key_type << " idx, "
                        << val_type << " d"
                 << ");"
                 << endl;

    (*m_ostream) << val_type << " *"
                 << "Ref" << "_" << map_translation.name
                 << "(" << map_translation.type << " *a, "
                        << key_type << " idx"
                 << ");"
                 << endl;

    return true;
}

// -------------------------------------------------------------------------- //

void FunctionForwardDeclVisitor::endVisit(ContractDefinition const&)
{
    m_translator.exit_scope();
}

void FunctionForwardDeclVisitor::endVisit(VariableDeclaration const&)
{
    m_translator.exit_scope();
}

void FunctionForwardDeclVisitor::endVisit(StructDefinition const&)
{
    m_translator.exit_scope();
}

// -------------------------------------------------------------------------- //

void FunctionForwardDeclVisitor::printArgs(CallableDeclaration const& _node)
{
    (*m_ostream) << "(";
    auto const& args = _node.parameters();
    for (unsigned int idx = 0; idx < args.size(); ++idx)
    {
        const auto &type = args[idx]->type();
        const auto &name = args[idx]->name();

        Translation type_translation = m_translator.translate(type);
        (*m_ostream) << type_translation.type << " " << name;
        if (idx + 1 < args.size())
        {
            (*m_ostream) << ", ";
        }
    }
    (*m_ostream) << ")";
}

void FunctionForwardDeclVisitor::printRetvals(CallableDeclaration const& _node)
{
    auto const& rettypes = _node.returnParameters();
    if (rettypes.empty())
    {
        (*m_ostream) << "void";
    }
    else if (rettypes.size() == 1)
    {
        (*m_ostream) << m_translator.translate(rettypes[0]->type()).type;
    }
    else
    {
        throw length_error("Multi-element return types unsupport.");
    }
}

// -------------------------------------------------------------------------- //

}
}
}
