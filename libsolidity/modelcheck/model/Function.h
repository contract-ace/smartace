/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
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

class CallState;
class ContractDependance;
class FunctionSpecialization;
class NewCallGraph;
class TypeConverter;

// -------------------------------------------------------------------------- //

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
		ContractDependance const& _dependance,
		CallState const& _statedata,
		NewCallGraph const& _newcalls,
		TypeConverter const& _converter,
		bool _add_sums,
		size_t _map_k,
		View _view,
		bool _fwd_dcl
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
	ContractDependance const& M_DEPENDANCE;
	CallState const& M_STATEDATA;
	NewCallGraph const& M_NEWCALLS;
	TypeConverter const& M_CONVERTER;

	bool const M_ADD_SUMS;
	size_t const M_MAP_K;

	View const M_VIEW;
	bool const M_FWD_DCL;

	// Helper structure to communicate parameters to the parameter generator.
	struct ParamTemplate
	{
		VarContext context;
		bool instrumentation;
		ASTPointer<const VariableDeclaration> decl;
	};

	// Formats all declarations as a C-function argument list. The given order
	// of arguments is maintained. If a scope is provided, then the arguments
	// are assumed to be of a stateful Solidity method, bound to structures of
	// the given type.
	CParams generate_params(
		std::vector<ParamTemplate> const& _args,
		ContractDefinition const* _scope,
		ASTPointer<VariableDeclaration> _dest
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
