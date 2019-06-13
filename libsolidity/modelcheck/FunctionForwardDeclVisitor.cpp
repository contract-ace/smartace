/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

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
        // Generates the AST for a constructor which acts as
        // ContractCompiler::initializeStateVariables.
        ASTPointer<ASTString> epsilon = make_shared<string>("");
        ASTPointer<ParameterList> empty_param_list = make_shared<ParameterList>(
            _node.location(),
            vector<ASTPointer<VariableDeclaration>>{}
        );
        vector<ASTPointer<Statement>> initializations;
        // TODO: populate body
        FunctionDefinition default_ctor_def(
            _node.location(),
            epsilon,
            Declaration::Visibility::Public,
            StateMutability::NonPayable,
            true,
            epsilon,
            empty_param_list,
            vector<ASTPointer<ModifierInvocation>>{},
            empty_param_list,
            make_shared<Block>(
                _node.location(),
                epsilon,
                initializations
            )
        );
        default_ctor_def.accept(*this);
    }
    return true;
}

bool FunctionForwardDeclVisitor::visit(StructDefinition const& _node)
{
    m_translator.enter_scope(_node);
    // Generates the AST for for a struct's default constructor.
    ASTPointer<ASTString> epsilon = make_shared<string>("");
    ASTPointer<ParameterList> empty_param_list = make_shared<ParameterList>(
        _node.location(),
        vector<ASTPointer<VariableDeclaration>>{}
    );
    vector<ASTPointer<Statement>> initializations;
    // Generates the argument list.
    vector<ASTPointer<VariableDeclaration>> params;
    for (const auto &param : _node.members())
    {
        const auto &type = param->type();
        if (!dynamic_cast<const ArrayType *>(type) &&
            !dynamic_cast<const MappingType *>(type))
        {
            params.push_back(param);
        }
    }
    // TODO: populate body
    FunctionDefinition default_ctor_def(
        _node.location(),
        epsilon,
        Declaration::Visibility::Public,
        StateMutability::Pure,
        true,
        epsilon,
        make_shared<ParameterList>(_node.location(), params),
        vector<ASTPointer<ModifierInvocation>>{},
        empty_param_list,
        make_shared<Block>(
            _node.location(),
            epsilon,
            initializations
        )
    );
    default_ctor_def.accept(*this);
    return true;
}

bool FunctionForwardDeclVisitor::visit(FunctionDefinition const& _node)
{
    // Produces return type.
    if (_node.isConstructor())
    {
        (*m_ostream) << m_translator.scope().type;
    }
    else
    {
        auto rettype = _node.functionType(false)->returnParameterTypes();
        if (rettype.empty())
        {
            (*m_ostream) << "void";
        }
        else if (rettype.size() == 1)
        {
            (*m_ostream) << m_translator.translate(rettype[0]).type;
        }
        else
        {
            throw length_error("Multi-element return types unsupport.");
        }
    }

    (*m_ostream) << " ";

    // Produces name of method.
    (*m_ostream) << ((_node.isConstructor()) ? "Ctor" : "Method")
                 << "_"
                 << m_translator.scope().name;
    if (!_node.isConstructor())
    {
        (*m_ostream) << "_" << _node.name();
    }

    // Produces the argument list.
    auto argtypes = _node.functionType(false)->parameterTypes();
    auto argnames = _node.functionType(false)->parameterNames();
    (*m_ostream) << "(";
    for (unsigned int idx = 0; idx < argtypes.size(); ++idx)
    {
        const auto &type = argtypes[idx];
        const auto &name = argnames[idx];

        Translation type_translation = m_translator.translate(type);
        (*m_ostream) << type_translation.type << " " << name;
        if (idx + 1 < argtypes.size())
        {
            (*m_ostream) << ", ";
        }
    }
    (*m_ostream) << ");";

    (*m_ostream) << endl;

    return false;
}

bool FunctionForwardDeclVisitor::visit(ModifierDefinition const& _node)
{
    (*m_ostream) << "M " << _node.name() << endl;
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

}
}
}
