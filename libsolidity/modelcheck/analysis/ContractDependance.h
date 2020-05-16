/**
 * TODO
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>

#include <map>
#include <set>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * The contract dependance is a second pass over the contract construction
 * graph. It is compared against a model (a list of contracts to model) and then
 * uses these contracts to determine the structures and methods we require to
 * resolve all calls.
 */
class ContractDependance
{
public:
    /**
     * Default mode for the contract dependance graph. The model is scanned to
     * extract all required structures and calls. Downcasting of state variable
     * contracts is handled by the provided call graph.
     */
    ContractDependance(
        std::vector<ContractDefinition const*> _model,
        NewCallGraph const& _graph
    );

    /**
     * A testing constructor which registers all contracts, functions and
     * structures as necessary.
     */
    ContractDependance(SourceUnit const& _srcs);

    /**
     * Returns true if the contract is ever used
     */
    bool is_deployed(ContractDefinition const* _actor) const;

private:
    /**
     * Extracts structures, contracts, and functions required by the given
     * contract.
     */
    void analyze_actors(
        ContractDefinition const& _actor,
        NewCallGraph const& _graph
    );

    /**
     * Extracts contracts required by this state variable.
     */
    void analyze_var(
        VariableDeclaration const& _var,
        NewCallGraph const& _graph
    );

    std::set<ContractDefinition const*> m_contracts;
};

// -------------------------------------------------------------------------- //

}
}
}
