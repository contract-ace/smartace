/**
 * Used to generate the main function scheduler. This constructs actors in the
 * model, initializes the restricted address space, manages the global state of
 * the simulation, and schedules the order of transactions.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/scheduler/ActorModel.h>
#include <libsolidity/modelcheck/scheduler/AddressSpace.h>
#include <libsolidity/modelcheck/scheduler/StateGenerator.h>

#include <memory>
#include <ostream>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AnalysisStack;
class FunctionSpecialization;
class NondetSourceRegistry;

// -------------------------------------------------------------------------- //

/**
 * Prints a forward declaration for the variable decalarations of contract,
 * globalstate and nextGS in main function.
 */
class MainFunctionGenerator
{
public:
    // Constructs a printer for all function forward decl's required by the ast.
    MainFunctionGenerator(
        bool _lockstep_time,
        std::shared_ptr<AnalysisStack const> _stack,
        std::shared_ptr<NondetSourceRegistry> _nd_reg
    );

    // Prints the main function.
    void print(std::ostream& _stream);

private:
    std::shared_ptr<AnalysisStack const> m_stack;

    std::shared_ptr<NondetSourceRegistry> m_nd_reg;

    // Stores data required to handle addresses.
    AddressSpace m_addrspace;

    // Stores data required to generate or repopulate the global state.
    StateGenerator m_stategen;

    // Stores data required to handle contract instances.
    ActorModel m_actors;

    // Expands and applies interference to all mappings in _decl. Assumes that
    // _decl is a sub-structure in _contract, that _path is the path to _decls,
    // and _block is the destination for the statements. The _display field is
    // a human readable version of the map for playback.
    void expand_interference(
        std::string _display,
        CExprPtr _path,
        FlatContract const& _contract,
        CBlockList & _block,
        VariableDeclaration const* _decl
    );

    // For each method on each contract, this will generate a case for the
    // switch block. Note that _args have been initialized first by
    // analyze_decls.
    CBlockList build_case(
        FunctionSpecialization const& _spec, std::shared_ptr<CVarDecl const> _id
    );

    // Helper method to format and log a call selection. The log statement is
    // appended to _block and describes an invocation of _call using _id as the
    // context.
    static void log_call(
        CBlockList & _block,
        CIdentifier const& _id,
        FunctionSpecialization const& _call
    );
};

// -------------------------------------------------------------------------- //

}
}
}
