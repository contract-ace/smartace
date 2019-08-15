/**
 * @date 2019
 * First-pass visitor for generating the CallState of Solidity in C models,
 * which consist of the struct of CallState.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <ostream>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Prints a forward declaration for the struct of CallState.
 */
class CallState : public ASTConstVisitor
{
public:
    // Constructs a printer for all contract forward decl's required by the ast.
    CallState(
        ASTNode const& _ast,
		TypeConverter const& _converter,
		bool _forward_declare
    );

    // Prints the struct of CallState.
    void print(std::ostream& _stream);

protected:
    void endVisit(ContractDefinition const& _node) override;

private:
	ASTNode const& m_ast;
	TypeConverter const& m_converter;
	std::ostream* m_ostream = nullptr;

	const bool m_forward_declare;
};

}
}
}
