/**
 * Converter from Solidity methods to SmartACE C functions.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Details.h>

#include <map>
#include <ostream>
#include <utility>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AnalysisStack;
class FlatContract;
class FunctionSpecialization;
class Structure;

// -------------------------------------------------------------------------- //

using SolDeclList = std::vector<ASTPointer<VariableDeclaration>>;

/**
 * Prints a forward declaration for each explicit (member function) and implicit
 * (default constructor, map accessor, etc.) Solidity function, according to the
 * SmartACE model.
 */
class FunctionConverter
{
public:
	// Specifies the class of methods to print.
	enum class View { FULL, INT, EXT };

    // Constructs a printer for all functions in the model.
    FunctionConverter(
		std::shared_ptr<AnalysisStack> _stack,
		bool _add_sums,
		size_t _map_k,
		View _view,
		bool _forward_declare
    );

    // Prints all user-defined functions, and implicit utility functions such as
	// constructors and map accessors.
    void print(std::ostream& _stream);

private:
	static std::shared_ptr<CIdentifier> const TMP;

	std::ostream* m_ostream = nullptr;

	bool const M_ADD_SUMS;
	size_t const M_MAP_K;

	View const M_VIEW;
	bool const M_FWD_DCL;

	std::shared_ptr<AnalysisStack> m_stack;

	std::set<std::pair<void const*, void const*>> m_visited;

	// Formats all Solidity arguments (_decls) as a c-function argument list.
	// If _scope is set, the function is assumed to be a method of _scope. The
	// _context and _instrumented pass to VariableScopeDeclaration::rewrite.
	CParams generate_params(
		SolDeclList const& _decls,
		ContractDefinition const* _scope,
		ASTPointer<VariableDeclaration> _dest,
		VarContext _context = VarContext::STRUCT,
		bool _instrumeneted = false
	);

	// Writes all utility methods associated with _mapping.
	void generate_mapping(Mapping const& _mapping);

	// Writes all utility methods associated with _struct.
	void generate_structure(Structure const& _struct);

	// Prints all inherited methods for function _func of contract _contract.
	void generate_method(
		FlatContract const& _contract, FunctionDefinition const& _func
	);

	// Prints the function specialization given by _spec.
	void generate_function(FunctionSpecialization const& _spec);

	// Prints the function specialization given by _spec, where the return value
	// has stringified type _rv_type. If _rv_is_ptr is true, the value is
	// returned by reference.
	std::string handle_function(
		FunctionSpecialization const& _spec,
		std::string _rv_type,
		bool _rv_is_ptr
	);

	// Recursively expands the hierarchy of initializations for _for, starting
	// from base contract _initialized.
	std::string handle_contract_initializer(
    	ContractDefinition const& _initialized, ContractDefinition const& _for
	);
};

// -------------------------------------------------------------------------- //

}
}
}
