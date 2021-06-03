/**
 * Used to generate the main function scheduler. This constructs actors in the
 * model, initializes the restricted address space, manages the global state of
 * the simulation, and schedules the order of transactions.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
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
    // Specifies how invariants should be instrumented:
    // - None: Invariants are unused. All mapping entries are non-deterministic.
    // - Unchecked: Invariants are assumed but never assert.
    // - Checked: Invariants assumed and asserted. Requires an extra 'client'.
    enum class InvarRule { None, Unchecked, Checked };

    // Specifies the specificity of invariants, if instrumented at all:
    // - Universal: All users have a single invariant, including implicit users.
    // - Singleton: All users, except implicit users, share a single invariant.
    // - RoleBased: An invariant exists for each rule.
    enum class InvarType { Universal, Singleton, RoleBased };

    // Constructs a printer for all function forward decl's required by the ast.
    MainFunctionGenerator(
        bool _lockstep_time,
        InvarRule _invar_rule,
        InvarType _invar_type,
        bool _infer_invar,
        std::shared_ptr<AnalysisStack const> _stack,
        std::shared_ptr<NondetSourceRegistry> _nd_reg
    );

    // Declares are invariants used by the bundle.
    void print_invariants(std::ostream& _stream);

    // Prints global declarations.
    void print_globals(std::ostream& _stream);

    // Prints the main function.
    void print_main(std::ostream& _stream);

private:
    // Records mapping data for invariant instrumentation.
    struct MapData
    {
        size_t id;
        CExprPtr path;
        MapDeflate::Record entry;
        std::string display;
    };
    std::vector<MapData> m_maps;

    // Analysis results.
    std::shared_ptr<AnalysisStack const> m_stack;

    // Used to non-deterministically initialize fields.
    std::shared_ptr<NondetSourceRegistry> m_nd_reg;

    // Stores data required to handle addresses.
    AddressSpace m_addrspace;

    // Stores data required to generate or repopulate the global state.
    StateGenerator m_stategen;

    // Stores data required to handle contract instances.
    ActorModel m_actors;

    // The invariant placement used by the harness.
    InvarRule m_invar_rule;

    // The invariant type used by the harness.
    InvarType m_invar_type;

    // If true, then checked invariants are also inferred.
    bool m_infer_invar;

    // Records all mappings within _maps. The list is computed recursively,
    // interating over each declaration within _contract. This assumes that
    // _decl is a substructure in _contract with path given by _path.
    void identify_maps(
        CExprPtr _path,
        FlatContract const& _contract,
        std::string _display,
        VariableDeclaration const* _decl
    );

    // Expands and applies interference to all mappings.
    CBlockList expand_interference();

    // Expands and checks that interference is closed for all mappings.
    CBlockList expand_interference_checks();

    //
    void apply_invariant(
        CBlockList &_block, bool _assert, CExprPtr _data, MapData &_map
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
