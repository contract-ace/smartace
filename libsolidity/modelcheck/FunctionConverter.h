/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <list>
#include <ostream>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Prints a forward declaration for each explicit (member function) and implicit
 * (default constructor, map accessor, etc.) Solidity function, according to the
 * C model.
 */
class FunctionConverter : public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    FunctionConverter(
        ASTNode const& _ast,
		bool _forward_declare
    );

    // Prints each for declaration once, in some order.
    void print(std::ostream& _stream);

	bool visit(ContractDefinition const& _node) override;
	bool visit(StructDefinition const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	bool visit(ModifierDefinition const& _node) override;
	bool visit(VariableDeclaration const& _node) override;
	bool visit(Mapping const& _node) override;

	void endVisit(ContractDefinition const&) override;
	void endVisit(VariableDeclaration const&) override;
	void endVisit(StructDefinition const&) override;

private:
	ASTNode const* m_ast;
	std::ostream* m_ostream = nullptr;

	TypeTranslator m_translator;

	const bool m_forward_declare;

	// Abstractions to handle a general CallableDeclaration.
	void printArgs(CallableDeclaration const& _node, bool _pass_state);
	void printRetvals(CallableDeclaration const& _node);
};

}
}
}
