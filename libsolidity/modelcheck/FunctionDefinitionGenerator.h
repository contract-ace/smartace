/**
 * @date 2019
 * Helper class to dynamically generate function nodes.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Utility to easily build AST function nodes inline.
 */
class FunctionDefinitionGenerator
{
public:
    // Creates a no-op, void function. It will take on the source location of its AST
    // node, along with the ctor value provided.
    FunctionDefinitionGenerator(
        ASTNode const& _srcnode,
        bool _is_ctor
    );

    // Simple API to extend the interface or body of the function.
    void push_arg(ASTPointer<VariableDeclaration> var);
    void push_retval(ASTPointer<VariableDeclaration> var);
    void push_statement(ASTPointer<Statement> stmt);

    // Produces a Solidity function definition which meets the given specifications.
    ASTPointer<FunctionDefinition> generate() const;

private:
    const langutil::SourceLocation LOC;
    const bool IS_CTOR;

    std::vector<ASTPointer<VariableDeclaration>> m_args;
    std::vector<ASTPointer<VariableDeclaration>> m_retvals;
    std::vector<ASTPointer<Statement>> m_statements;
};

}
}
}
