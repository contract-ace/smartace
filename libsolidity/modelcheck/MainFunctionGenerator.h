/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
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
 * Prints a forward declaration for the variable decalarations of contract,
 * globalstate and nextGS in main function.
 * 
 * TODO(scott.wesley): As with the original implementation, this failures when
 *                     there are two (or n) source units. This will generate two
 *                     (or n) entry-points. A solution similar to TypeTranslator
 *                     is needed.
 */
class MainFunctionGenerator: public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunctionGenerator(
        SourceUnit const& _ast,
		TypeConverter const& _converter
    );

    // Prints the main function.
    void print(std::ostream& _stream);

private:
    CExprPtr const NULL_LIT;

	SourceUnit const& m_ast;
	TypeConverter const& m_converter;

    CStmtPtr make_require(CExprPtr _cond);

    void analyze_decls(
        std::vector<ContractDefinition const*> const& _contracts, 
        std::map<VariableDeclaration const*, std::shared_ptr<CVarDecl>> & _dcls,
        std::map<FunctionDefinition const*, uint64_t> & _funcs);
};

}
}
}
