/**
 * @date 2019
 * First-pass visitor for converting Solidity AST's to models in C.
 */

#include <libsolidity/modelcheck/ASTForwardDeclVisitor.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

ASTForwardDeclVisitor::ASTForwardDeclVisitor(
    ASTNode const& _ast
): m_ast(&_ast)
{
}

void ASTForwardDeclVisitor::print(ostream& _stream)
{
    m_ostream = &_stream;
    m_ast->accept(*this);
    m_ostream = nullptr;
}

bool ASTForwardDeclVisitor::visit(ContractDefinition const& _node)
{
    m_translator.enter_scope(_node);
    (*m_ostream) << m_translator.translate(_node) << ";" << endl;
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

bool ASTForwardDeclVisitor::visit(StructDefinition const& _node)
{
    m_translator.enter_scope(_node);
    (*m_ostream) << m_translator.translate(_node) << ";" << endl;
    // Generates the AST for for a struct's default constructor.
    ASTPointer<ASTString> epsilon = make_shared<string>("");
    ASTPointer<ParameterList> empty_param_list = make_shared<ParameterList>(
        _node.location(),
        vector<ASTPointer<VariableDeclaration>>{}
    );
    vector<ASTPointer<Statement>> initializations;
    // TODO: populate body
    // TODO: populate arg list
    FunctionDefinition default_ctor_def(
        _node.location(),
        epsilon,
        Declaration::Visibility::Public,
        StateMutability::Pure,
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
    return true;
}

bool ASTForwardDeclVisitor::visit(FunctionDefinition const& _node)
{
    // Produces return type.
    if (_node.isConstructor())
    {
        (*m_ostream) << m_translator.scope_type();
    }
    else
    {
        auto ftype = _node.functionType(false);
        if (ftype->returnParameterTypes().size() == 0)
        {
            (*m_ostream) << "void";
        }
        else
        {
            // TODO
        }
    }

    (*m_ostream) << " ";

    // Produces name of method.
    (*m_ostream) << ((_node.isConstructor()) ? "Ctor" : "Method")
                 << "_"
                 << m_translator.scope_name();
    if (!_node.isConstructor())
    {
        (*m_ostream) << "_" << _node.name();
    }

    (*m_ostream) << endl;

    return false;
}

bool ASTForwardDeclVisitor::visit(ModifierDefinition const& _node)
{
    (*m_ostream) << "M " << _node.name() << endl;
    return false;
}

bool ASTForwardDeclVisitor::visit(VariableDeclaration const& _node)
{
    m_translator.enter_scope(_node);
    return true;
}

bool ASTForwardDeclVisitor::visit(Mapping const& _node)
{
    (*m_ostream) << m_translator.translate(_node) << ";" << endl;
    // TODO: print helper methods.
    return true;
}

void ASTForwardDeclVisitor::endVisit(ContractDefinition const&)
{
    m_translator.exit_scope();
}

void ASTForwardDeclVisitor::endVisit(VariableDeclaration const&)
{
    m_translator.exit_scope();
}

void ASTForwardDeclVisitor::endVisit(StructDefinition const&)
{
    m_translator.exit_scope();
}

}
}
}
