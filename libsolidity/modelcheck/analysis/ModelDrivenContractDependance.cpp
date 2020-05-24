/**
 * @date 2020
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 * 
 * Implements the model-driven specialization.
 */


#include <libsolidity/modelcheck/analysis/ContractDependance.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ModelDrivenContractDependance::SuperChainExtractor::SuperChainExtractor(
    FunctionDefinition const& _call
): m_superchain({ &_call }), m_base(_call)
{
}

// -------------------------------------------------------------------------- //

void ModelDrivenContractDependance::SuperChainExtractor::endVisit(
    FunctionCall const& _node
)
{
    // If the type isn't a function this isn't a super call.
	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND != FunctionCallKind::FunctionCall) return;

    // Otherwise checks if this is a contract method call.
    FunctionCallAnalyzer calldata(_node);
    if (calldata.classify() == FunctionCallAnalyzer::CallGroup::Method)
    {
        // If the name doesn't match, move on.
        if (m_base.func().name() != calldata.decl().name()) return;

        // Otherwise, handles the super call.
        if (calldata.is_super())
        {
            m_superchain.insert(&calldata.decl());
            calldata.decl().body().accept(*this);
        }
    }
}

// -------------------------------------------------------------------------- //

ModelDrivenContractDependance::ModelDrivenContractDependance(
    vector<ContractDefinition const*> _model, NewCallGraph const& _graph
): DependencyAnalyzer(_model)
{
    // Iterates over each contract in the model, where the model is expanding.
    for (size_t i = 0; i < _model.size(); ++i)
    {
        m_contracts.insert(_model[i]);

        // Explores the inheritance tree for contract state variables.
        for (auto base : _model[i]->annotation().linearizedBaseContracts)
        {
            // If this is an interface it declares nothing of interest.
            if (base->isInterface()) break;

            // Otherwise, we iterate the variables.
            for (auto var : base->stateVariables())
            {
                // If the variable is a contract, and new, it is in the model.
                auto vartype = var->annotation().type->category();
                if (vartype == Type::Category::Contract)
                {
                    auto const& specialized = _graph.specialize(*var);
                    if (m_contracts.find(&specialized) == m_contracts.end())
                    {
                        _model.push_back(&specialized);
                    }
                }
            }
        }
    }
}

// -------------------------------------------------------------------------- //

ContractDependance::FuncInterface
    ModelDrivenContractDependance::get_interfaces_for(
        ContractDefinition const* _contract
) const
{
    ContractDependance::FuncInterface interfaces;

    // Iterates through each base class of this contract.
    set<string> methods;
    for (auto base : _contract->annotation().linearizedBaseContracts)
    {
        // If the class is an interface, this is the end of analysis.
        if (base->isInterface()) break;

        // Otherwise, analyze each function.
        for (auto func : base->definedFunctions())
        {
            // Skips over unimplemented methods.
            if (!func->isImplemented()) continue;
    
            // If this name has already been seen, it's a base implementation.
            if (!methods.insert(func->name()).second) continue;

            interfaces.push_back(func);
        }
    }

    return interfaces;
}

// -------------------------------------------------------------------------- //

ContractDependance::FunctionSet
    ModelDrivenContractDependance::get_superchain_for(
        ContractDependance::FuncInterface _interfaces,
        FunctionDefinition const* _func
) const
{
    SuperChainExtractor extractor(*_func);
    for (auto func : _interfaces) func->accept(extractor);
    return extractor.m_superchain;
}

// -------------------------------------------------------------------------- //

}
}
}
