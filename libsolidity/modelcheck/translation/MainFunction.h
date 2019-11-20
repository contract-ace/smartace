/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <list>
#include <ostream>
#include <utility>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * Prints a forward declaration for the variable decalarations of contract,
 * globalstate and nextGS in main function.
 */
class MainFunctionGenerator: public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunctionGenerator(
        std::list<ContractDefinition const *> const& _model,
        NewCallGraph const& _new_graph,
        TypeConverter const& _converter
    );

    // Integrates a source unit with the main function.
    void record(SourceUnit const& _ast);

    // Prints the main function.
    void print(std::ostream& _stream);

private:
    // Encodes the structures needed to represent a contract instance as a unique
    // actor in the model.
    struct Actor
    {
        Actor(
            TypeConverter const& _converter,
            ContractDefinition const* _contract,
            CExprPtr _path,
            TicketSystem<uint16_t> & _cids,
            TicketSystem<uint16_t> & _fids
        );

        ContractDefinition const* contract;
        std::shared_ptr<CVarDecl> decl;
        std::map<FunctionDefinition const*, size_t> fnums;
        std::map<VariableDeclaration const*, std::shared_ptr<CVarDecl>> fparams;
        CExprPtr path;
    };

    // The list of contracts requested for the model. If empty, then it one of
    // each contract is instantiated.
    std::list<ContractDefinition const*> const& m_model;

    NewCallGraph const& m_new_graph;

    // Primed typpe converter.
	TypeConverter const& m_converter;

    // A list of all contracts observed by this translator.
    std::list<ContractDefinition const*> m_contracts;

    CStmtPtr make_require(CExprPtr _cond);

    // Performs preprocessing of the contract list. For each parameter, a
    // declaration will be generated, and registered within _dcls. For each
    // contract, another declaration will be generated, and added to _defs. A
    // unique identifier will be given to each function for use within the
    // switch block. This is recorded within _funcs.
    std::list<Actor> analyze_decls(
        std::vector<ContractDefinition const*> const& _contracts
    ) const;

    // Extends analyze_decls to children. _actors will be mutated in-place.
    // _path will accumulate the path to the current parent, starting from a top
    // level contract.
    void analyze_nested_decls(
        std::list<Actor> & _actors,
        CExprPtr _path,
        ContractDefinition const* _parent,
        TicketSystem<uint16_t> & _cids,
        TicketSystem<uint16_t> & _fids
    ) const;

    // Consumes a contract declaration, and initializes it through
    // non-deterministic construction.
    CStmtPtr init_contract(
        ContractDefinition const& _contract,
        std::shared_ptr<const CVarDecl> _id,
        std::shared_ptr<const CVarDecl> _state
    );

    // For each method on each contract, this will generate a case for the
    // switch block. Note that _args have been initialized first by
    // analyze_decls.
    CBlockList build_case(
        FunctionDefinition const& _def,
        std::map<VariableDeclaration const*, std::shared_ptr<CVarDecl>> & _args,
        std::shared_ptr<const CVarDecl> _id,
        std::shared_ptr<const CVarDecl> _state
    );

    // Generate the instructions required to update the call state.
    void update_call_state(
        CBlockList & _stmts,
        std::shared_ptr<const CVarDecl> _state
    );

    static CExprPtr get_nd_byte(std::string const& _msg);
};

}
}
}
