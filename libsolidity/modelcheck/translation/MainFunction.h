/**
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/MapIndex.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
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
 * Prints a forward declaration for the variable decalarations of contract,
 * globalstate and nextGS in main function.
 */
class MainFunctionGenerator: public ASTConstVisitor
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunctionGenerator(
        bool _lockstep_time,
	    MapIndexSummary const& _addrdata,
        std::list<ContractDefinition const *> const& _model,
        NewCallGraph const& _new_graph,
        CallState const& _statedata,
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
        std::list<FunctionSpecialization> specs;
        std::shared_ptr<CVarDecl> decl;
        std::map<FunctionDefinition const*, size_t> fnums;
        std::map<VariableDeclaration const*, std::shared_ptr<CVarDecl>> fparams;
        CExprPtr path;
    };

    // If true, block and timpstamp move together.
    bool const M_LOCKSTEP_TIME;

    // If true, the zero special constant is in use.
    bool const M_USES_ZERO;

    // Stores all parameters over the address space.
    MapIndexSummary const& m_addrdata;

    // The list of contracts requested for the model. If empty, then it one of
    // each contract is instantiated.
    std::list<ContractDefinition const*> const& m_model;

    NewCallGraph const& m_new_graph;
    CallState const& m_statedata;
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
    void init_contract(
        CBlockList & _stmts,
        ContractDefinition const& _contract,
        std::shared_ptr<const CVarDecl> _id
    );

    // For each method on each contract, this will generate a case for the
    // switch block. Note that _args have been initialized first by
    // analyze_decls.
    CBlockList build_case(
        FunctionSpecialization const& _spec,
        std::map<VariableDeclaration const*, std::shared_ptr<CVarDecl>> & _args,
        std::shared_ptr<const CVarDecl> _id
    );

    // Generate the instructions required to update the call state.
    void update_call_state(
        CBlockList & _stmts,
        std::list<std::shared_ptr<CMemberAccess>> const& _addresses,
        std::list<CExprPtr> const& _addrvars
    );

    // Generates a value for a payable method.
    void set_payment_value(CBlockList & _stmts);

    static CExprPtr get_nd_byte(std::string const& _msg);
    static CExprPtr get_nd_range(uint8_t _l, uint8_t _u, std::string const& _msg);
};

}
}
}
