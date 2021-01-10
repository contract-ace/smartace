/**
 * Generates the scheduler code required to instantiate and manage actors.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/codegen/Details.h>

#include <list>
#include <map>
#include <memory>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AddressSpace;
class AnalysisStack;
class FlatContract;
class FunctionSpecialization;
class MapIndexSummary;
class NondetSourceRegistry;
class StateGenerator;

// -------------------------------------------------------------------------- //

/**
 * A structure used to summarize each contract in the system. This includes
 * metadata and also declarations.
 */
struct Actor
{
    Actor(
        std::shared_ptr<AnalysisStack const> _stack,
        std::shared_ptr<FlatContract const> _contract,
        size_t _id,
        CExprPtr _path
    );

    // The underlying contract.
    std::shared_ptr<FlatContract const> contract;

    // A variable declaration used to maintain the actor in the harness.
    std::shared_ptr<CVarDecl> decl;

    // Specializations of all member funtions
    std::list<FunctionSpecialization> specs;

    // Maintains an access path, from parent contract to child contract.
    CExprPtr path;

    // If true, the actor has been used to spawn a child contract.
    bool has_children;
};

// -------------------------------------------------------------------------- //

/**
 * Provides utilities to summarize, initialize, and maintain contract instances,
 * known as actors.
 */
class ActorModel
{
public:
    //
    ActorModel(
        std::shared_ptr<AnalysisStack const> _stack,
        std::shared_ptr<NondetSourceRegistry> _nd_reg
    );

    // Appends a declaration for each actor onto _block.
    void declare(CBlockList & _block) const;

    // Writes an initialization call to _block, for each actor. Nested actors
    // are not initialized at this level. Initialization is performed with
    // non-deterministic parameters. _statedata and _stategen are used to set
    // the block and message state.
    void initialize(
        CBlockList & _block, StateGenerator const& _stategen
    ) const;

    // Appends statements onto _block to allocate addresses for each actor. The
    // addresses are requested from _addrspace. The call throws if
    // m_actors.size() exceeds the number of available addresses.
    void assign_addresses(CBlockList & _block, AddressSpace & _addrspace) const;

    // Returns a list of contract address declarations.
    std::list<std::shared_ptr<CMemberAccess>> const& vars() const;

    // Allow read-only access to this contract's actors
    std::list<Actor> const& inspect() const;

private:
	std::shared_ptr<AnalysisStack const> m_stack;

    std::shared_ptr<NondetSourceRegistry> m_nd_reg;

    // The list of actors, which is populated after setup.
    std::list<Actor> m_actors;

    // An anonymous list of contract address member variables.
    std::list<std::shared_ptr<CMemberAccess>> m_addrvar;

    // Extends setup to children. _path will accumulate the path to the current
    // parent, starting from a top level contract. _allocs is used to find all
    // children while _dependance is used to populate interface methods.
    void recursive_setup(Actor & _parent);
};

// -------------------------------------------------------------------------- //

}
}
}
