/**
 * @date 2019
 * First-pass visitor for converting Solidity ADT's into structs in C.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/Types.h>
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
 * Interprets the AST in terms of its C model, and prints forward declarations
 * for each of structures.
 */
class ADTConverter : public ASTConstVisitor
{
public:
    // Constructs a printer for all ADT's required by the ast's c model. The
	// converter should provide translations for all typed ASTNodes. If forward
	// declare is set, then the structure bodies are not generated.
    ADTConverter(
        ASTNode const& _ast,
		TypeConverter const& _converter,
		size_t _map_k,
		bool _forward_declare
    );

    // Prints each ADT declaration once, in some order.
    void print(std::ostream& _stream);

protected:
	bool visit(ContractDefinition const& _node) override;

	void endVisit(VariableDeclaration const& _node) override;
	void endVisit(Mapping const& _node) override;
	void endVisit(StructDefinition const& _node) override;

private:
	ASTNode const& M_AST;
	TypeConverter const& M_CONVERTER;

	size_t const M_MAP_K;
	bool const M_FORWARD_DECLARE;

	std::ostream* m_ostream = nullptr;

	std::set<ContractDefinition const*> m_built;
};

}
}
}
