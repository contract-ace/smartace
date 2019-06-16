/**
 * @date 2019
 * Helper class to dynamically generate function nodes.
 */

#include <libsolidity/modelcheck/FunctionDefinitionGenerator.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

FunctionDefinitionGenerator::FunctionDefinitionGenerator(
    ASTNode const& _srcnode,
    bool _is_ctor
): LOC(_srcnode.location()), IS_CTOR(_is_ctor)
{
}

// -------------------------------------------------------------------------- //

void FunctionDefinitionGenerator::push_arg(ASTPointer<VariableDeclaration> var)
{
    m_args.push_back(std::move(var));
}

void FunctionDefinitionGenerator::push_retval(ASTPointer<VariableDeclaration> var)
{
    m_retvals.push_back(std::move(var));
}

void FunctionDefinitionGenerator::push_statement(ASTPointer<Statement> stmt)
{
    m_statements.push_back(stmt);
}

// -------------------------------------------------------------------------- //

ASTPointer<FunctionDefinition> FunctionDefinitionGenerator::generate() const
{
    ASTPointer<ASTString> epsilon = make_shared<string>("");
    return std::make_shared<FunctionDefinition>(
        LOC,
        epsilon,
        Declaration::Visibility::Public,
        StateMutability::Pure,
        IS_CTOR,
        epsilon,
        make_shared<ParameterList>(LOC, m_args),
        vector<ASTPointer<ModifierInvocation>>{},
        make_shared<ParameterList>(LOC, m_retvals),
        make_shared<Block>(LOC, epsilon, m_statements)
    );
}

}
}
}
