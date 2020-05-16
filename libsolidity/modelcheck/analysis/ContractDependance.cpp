/**
 * TODO
 */

#include <libsolidity/modelcheck/analysis/ContractDependance.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ContractDependance::ContractDependance(
    vector<ContractDefinition const*> _model,
    NewCallGraph const& _graph
)
{
    for (auto actor : _model)
    {
        analyze_actors(*actor, _graph);
    }
}

// -------------------------------------------------------------------------- //

ContractDependance::ContractDependance(SourceUnit const& _srcs)
{
    auto CONTRACTS = ASTNode::filteredNodes<ContractDefinition>(_srcs.nodes());
    for (auto contract : CONTRACTS)
    {
        m_contracts.insert(contract);
    }
}

// -------------------------------------------------------------------------- //

bool ContractDependance::is_deployed(ContractDefinition const* _actor) const
{
    return (m_contracts.find(_actor) != m_contracts.end());
}

// -------------------------------------------------------------------------- //

void ContractDependance::analyze_actors(
    ContractDefinition const& _actor,
    NewCallGraph const& _graph)
{
    if (!is_deployed(&_actor))
    {
        m_contracts.insert(&_actor);
        
        for (auto var : _actor.stateVariables())
        {
            analyze_var(*var, _graph);
        }
    }
}

// -------------------------------------------------------------------------- //

void ContractDependance::analyze_var(
    VariableDeclaration const& _var,
    NewCallGraph const& _graph
)
{
    if (_var.annotation().type->category() == Type::Category::Contract)
    {
        auto const& specialized = _graph.specialize(_var);
        analyze_actors(specialized, _graph);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
