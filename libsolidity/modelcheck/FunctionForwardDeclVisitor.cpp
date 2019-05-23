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

bool FunctionForwardDeclVisitor::visit(FunctionDefinition const& _node)
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

bool FunctionForwardDeclVisitor::visit(Mapping const&)
{
    // TODO: print helper methods.
    return true;
}

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

}
}
}
