/**
 * @date 2020
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 * 
 * Implements the model-driven specialization.
 */


#include <libsolidity/modelcheck/analysis/ContractDependance.h>

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
) : m_superchain({ &_call })
{
    _call.body().accept(*this);
}

// -------------------------------------------------------------------------- //

bool ModelDrivenContractDependance::SuperChainExtractor::visit(
    FunctionCall const& _node
)
{
    // If the type isn't a function this isn't a super call.
	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND != FunctionCallKind::FunctionCall) return true;

    // Otherwise checks if this is a contract method call.
    // TODO: this switch maps FunctionTypeKinds to SmartACE entities. Factor.
	FunctionCallAnalyzer calldata(_node);
    switch (calldata.type().kind())
    {
	case FunctionType::Kind::Internal:
	case FunctionType::Kind::External:
	case FunctionType::Kind::BareCall:
	case FunctionType::Kind::BareStaticCall:
        if (calldata.is_super())
        {
            m_superchain.push_back(&calldata.decl());
            calldata.decl().body().accept(*this);
            return false;
        }
        return true;
    default:
        return true;
    }
}

// -------------------------------------------------------------------------- //

ModelDrivenContractDependance::ModelDrivenContractDependance(
    vector<ContractDefinition const*> _model, NewCallGraph const& _graph
)
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
        ContractDefinition const* _ctrt
) const
{
    ContractDependance::FuncInterface interfaces;

    // Iterates through each base class of this contract.
    set<string> methods;
    for (auto base : _ctrt->annotation().linearizedBaseContracts)
    {
        // If the class is an interface, this is the end of analysis.
        if (base->isInterface()) break;

        // Otherwise, analyze each function.
        for (auto func : base->definedFunctions())
        {
            // If this name has already been seen, this is a superclass.
            if (!methods.insert(func->name()).second) continue;

            interfaces.push_back(func);
        }
    }

    return interfaces;
}

// -------------------------------------------------------------------------- //

ContractDependance::SuperCalls
    ModelDrivenContractDependance::get_superchain_for(
        FunctionDefinition const* _func
) const
{
    SuperChainExtractor extractor(*_func);
    return extractor.m_superchain;
}

// -------------------------------------------------------------------------- //

}
}
}
