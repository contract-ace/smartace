/**
 * @date 2019
 * First-pass visitor for generating Solidity the second part of main function,
 * which consist of printing cases in switch in main function.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <list>
#include <ostream>
#include <set>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Prints switch cases.
 */
class MainFunction_3 : public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunction_3(
        ASTNode const& _ast,
		TypeConverter const& _converter,
		bool _forward_declare
    );

    // Prints switch cases.
    void print(std::ostream& _stream);

protected:


  bool visit(FunctionDefinition const& _node) override;
  void endVisit(ContractDefinition const& _node) override;

private:
	ASTNode const& m_ast;
	TypeConverter const& m_converter;
	std::ostream* m_ostream = nullptr;

  int i=0;
	const bool m_forward_declare;

  // Prints switch cases.
	void print_args(
		std::vector<ASTPointer<VariableDeclaration>> const& _args, ASTNode const* _scope
	);
};

}
}
}
