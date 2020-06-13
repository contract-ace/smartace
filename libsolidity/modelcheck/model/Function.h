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
class FunctionSpecialization;

// -------------------------------------------------------------------------- //

using SolDeclList = std::vector<ASTPointer<VariableDeclaration>>;

/**
 * Prints a forward declaration for each explicit (member function) and implicit
 * (default constructor, map accessor, etc.) Solidity function, according to the
 * C model.
 */
class FunctionConverter : public ASTConstVisitor
{
public:
	// Specifies the class of methods to print.
	enum class View { FULL, INT, EXT };

    // Constructs a printer for all function forward decl's required by the ast.
    FunctionConverter(
        ASTNode const& _ast,
		std::shared_ptr<AnalysisStack> _stack,
		bool _add_sums,
		size_t _map_k,
		View _view,
		bool _forward_declare
    );

    // Prints each function-like declaration once, in some order. Special
	// functions, such as constructors and accessors are also generated.
    void print(std::ostream& _stream);

protected:
	bool visit(ContractDefinition const& _node) override;
	bool visit(StructDefinition const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	bool visit(ModifierDefinition const&) override;
	bool visit(Mapping const& _node) override;

private:
	static FunctionDefinition const PLACEHOLDER_FUNC;
	static std::shared_ptr<CIdentifier> const TMP;

	std::map<std::pair<size_t, size_t>, bool> m_handled;

	std::ostream* m_ostream = nullptr;

	ASTNode const& M_AST;

	bool const M_ADD_SUMS;
	size_t const M_MAP_K;

	View const M_VIEW;
	bool const M_FWD_DCL;

	std::shared_ptr<AnalysisStack> m_stack;

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

	// Helper function to avoid duplicate visits to a single specialization. If
	// the pair already exists, false is returned.
	bool record_pair(ASTNode const& inst, ASTNode const& user);

	// Determines whether or not to generate a function.
	void generate_function(FunctionSpecialization const& _spec);

	// Generates a layer of the contract constructor.
	std::string handle_function(
		FunctionSpecialization const& _spec,
		std::string _rv_type,
		bool _rv_is_ptr
	);

	// Recursively expands an initializer for a contract.
	std::string handle_contract_initializer(
    	ContractDefinition const& _initialized, ContractDefinition const& _for
	);
};

// -------------------------------------------------------------------------- //

}
}
}
