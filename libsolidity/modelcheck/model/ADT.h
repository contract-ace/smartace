/**
 * Converter from Solidity contracts, structures and mappings into SmartACE C
 * structs.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/AST.h>

#include <memory>
#include <ostream>
#include <set>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AnalysisStack;
class FlatContract;
class Structure;

// -------------------------------------------------------------------------- //

/**
 * Interprets the AST in terms of its C model, and prints forward declarations
 * for each of structures.
 */
class ADTConverter
{
public:
    // Constructs a printer for all ADT's required by the ast's c model. The
	// converter should provide translations for all typed ASTNodes. If forward
	// declare is set, then the structure bodies are not generated.
    ADTConverter(
		std::shared_ptr<AnalysisStack const> _stack,
		bool _add_sums,
		size_t _map_k,
		bool _forward_declare
    );

    // Prints each ADT declaration once, in some order.
    void print(std::ostream& _stream);

private:
	bool const M_ADD_SUMS;
	size_t const M_MAP_K;
	bool const M_FORWARD_DECLARE;

	std::shared_ptr<AnalysisStack const> m_stack;

	std::ostream* m_ostream = nullptr;

	std::set<void const*> m_built;
	
	// Prints all dependencies of _contract, and then the contract itself.
	void generate_contract(FlatContract const& _contract);

	// Prints _mapping.
	void generate_mapping(Mapping const& _mapping);

	// Prints all mapping dependencies of _structure, and then the structure
	// itself.
	void generate_structure(Structure const& _structure);
};

// -------------------------------------------------------------------------- //

}
}
}
