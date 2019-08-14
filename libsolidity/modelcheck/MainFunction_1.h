/**
 * @date 2019
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
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
 * Prints a forward declaration for the variable decalarations of contract, globalstate and nextGS in main function.
 */
class MainFunction_1 : public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunction_1(
        ASTNode const& _ast,
		TypeConverter const& _converter,
		bool _forward_declare
    );

    // Prints the main function.
    void print(std::ostream& _stream);

protected:

  bool visit(ContractDefinition const& _node) override;
  bool visit(FunctionDefinition const& _node) override;
  void endVisit(ContractDefinition const& _node) override;

private:
	ASTNode const& m_ast;
	TypeConverter const& m_converter;
	std::ostream* m_ostream = nullptr;

  int i = 0;
  bool access = false;

	const bool m_forward_declare;

	// Formats all declarations as a C-function argument list. The given order
	// of arguments is maintained. If a scope is provided, then the arguments
	// are assumed to be of a stateful Solidity method, bound to structures of
	// the given type. If values are defaulted to zero, then the constructor
	// will have a default value of zero for each parameter.
	void print_args(
		std::vector<ASTPointer<VariableDeclaration>> const& _args
	);
};

}
}
}
