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
    string name = _node.name();
    declare_struct_in_scope(name);
    push_scope(std::move(name));
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
    string name = _node.name();
    declare_struct_in_scope(name);
    push_scope(std::move(name));
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
    // Fully expands scope type.
    stringstream owner_oss;
    owner_oss << m_model_scope.front();
    for (auto scope = ++m_model_scope.begin(); scope != m_model_scope.end(); ++scope)
    {
        owner_oss << "_" << (*scope);
    }

    // Produces return type.
    if (_node.isConstructor())
    {
        (*m_ostream) << "struct " << owner_oss.str();
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
                 << owner_oss.str();
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
    push_scope(_node.name());
    return true;
}

bool ASTForwardDeclVisitor::visit(Mapping const&)
{
    if (m_map_depth > 0)
    {
        declare_struct_in_scope("submap" + to_string(m_map_depth));
    }
    // TODO: print helper methods.
    ++m_map_depth;
    return true;
}

void ASTForwardDeclVisitor::endVisit(ContractDefinition const&)
{
    pop_scope();
}

void ASTForwardDeclVisitor::endVisit(VariableDeclaration const&)
{
    pop_scope();
}

void ASTForwardDeclVisitor::endVisit(Mapping const&)
{
    --m_map_depth;
}

void ASTForwardDeclVisitor::endVisit(StructDefinition const&)
{
    pop_scope();
}

void ASTForwardDeclVisitor::declare_struct_in_scope(const string &name)
{
    (*m_ostream) << "struct ";
    for (const auto scope : m_model_scope)
    {
        (*m_ostream) << scope << "_";
    }
    (*m_ostream) << name << ";" << endl;
}

void ASTForwardDeclVisitor::push_scope(std::string scope)
{
    m_model_scope.push_back(std::move(scope));
}

void ASTForwardDeclVisitor::pop_scope()
{
    m_model_scope.pop_back();
}

}
}
}
