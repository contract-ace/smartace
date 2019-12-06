/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/utils/Types.h>
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
	// Specifies the class of methods to print.
	enum class View { FULL, INT, EXT };

    // Constructs a printer for all function forward decl's required by the ast.
    FunctionConverter(
        ASTNode const& _ast,
		CallState const& _statedata,
		TypeConverter const& _converter,
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
	static const FunctionDefinition PLACEHOLDER_FUNC;
	static const std::shared_ptr<CIdentifier> TMP;

	std::ostream* m_ostream = nullptr;

	ASTNode const& M_AST;
	CallState const& M_STATEDATA;
	TypeConverter const& M_CONVERTER;

	size_t const M_MAP_K;

	View const M_VIEW;
	bool const M_FWD_DCL;

	// Helper structure to communicate parameters to the parameter generator.
	struct ParamTmpl
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
		std::vector<ParamTmpl> const& _args, ASTNode const* _scope
	);

	// Generates a layer of the contract constructor.
	std::string handle_function(
		FunctionDefinition const& _func, ASTNode const& _scope
	);

	// Recursively expands an initializer for a contract.
	std::string handle_contract_initializer(
    	ContractDefinition const& _initialized, ContractDefinition const& _for
	);
};

}
}
}
