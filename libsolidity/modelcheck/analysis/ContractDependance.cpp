/**
 * @date 2020
 * A set of tools to analyze the dependance between contracts, their methods and
 * their structs.
 */

#include <libsolidity/modelcheck/analysis/ContractDependance.h>

#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CallReachAnalyzer::CallReachAnalyzer(FunctionDefinition const& _func)
{
    m_calls.insert(&_func);
    _func.body().accept(*this);
}

// -------------------------------------------------------------------------- //

bool CallReachAnalyzer::visit(IndexAccess const& _node)
{
    FlatIndex idx(_node);
    if (idx.base().annotation().type->category() == Type::Category::Mapping)
    {
        m_reads.insert(&idx.decl());
    }

    for (auto param : idx.indices()) param->accept(*this);
    idx.base().accept(*this);

    return false;
}

void CallReachAnalyzer::endVisit(FunctionCall const& _node)
{
    // If the type isn't a function this isn't a super call.
	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND != FunctionCallKind::FunctionCall) return;

    // Otherwise checks if this is a contract method call.
	FunctionCallAnalyzer calldata(_node);
    if (calldata.classify() == FunctionCallAnalyzer::CallGroup::Method)
    {
        if (m_calls.insert(&calldata.decl()).second)
        {
            calldata.decl().body().accept(*this);
        }
    }
}

ContractDependance::DependancyAnalyzer::DependancyAnalyzer(
    ContractDependance::ContractList _model
): m_model(move(_model))
{
}

// -------------------------------------------------------------------------- //

ContractDependance::ContractDependance(
    ContractDependance::DependancyAnalyzer const& _analyzer
) : m_contracts(_analyzer.m_contracts)
 ,  m_model(_analyzer.m_model)
{
    // Does per-contract analysis.
    for (auto contract : m_contracts)
    {
        // Finds all interfaces of this contract.
        m_interfaces[contract] = _analyzer.get_interfaces_for(contract);

        // Does per-interface analysis.
        for (auto interface : m_interfaces[contract])
        {
            // Finds supercalls if not already populated.
            if (m_superchain.find(interface) == m_superchain.end())
            {
                m_superchain[interface]
                    = _analyzer.get_superchain_for(interface);
            }

            // Computes ROI of the given interface.
            // TODO: reusing partial results would be nice.
            CallReachAnalyzer reach(*interface);
            m_functions.insert(reach.m_calls.begin(), reach.m_calls.end());
            m_callreach[interface] = std::move(reach.m_calls);
            m_mapreach[interface] = std::move(reach.m_reads);
        }
    }
}

// -------------------------------------------------------------------------- //

ContractDependance::ContractList const& ContractDependance::get_model() const
{
    if (m_model.empty())
    {
        throw std::runtime_error("ContractDependance: Model not available.");
    }
    return m_model;
}

// -------------------------------------------------------------------------- //

bool ContractDependance::is_deployed(ContractDefinition const* _actor) const
{
    return (m_contracts.find(_actor) != m_contracts.end());
}

// -------------------------------------------------------------------------- //

ContractDependance::SuperCalls const& ContractDependance::get_superchain(
    FunctionDefinition const* _func
) const
{
    auto res = m_superchain.find(_func);
    if (res != m_superchain.end())
    {
        return res->second;
    }
    throw std::runtime_error("Superchain requested on out-of-scope function.");
}

// -------------------------------------------------------------------------- //

ContractDependance::FuncInterface const& ContractDependance::get_interface(
    ContractDefinition const* _actor
) const
{
    auto res = m_interfaces.find(_actor);
    if (res != m_interfaces.end())
    {
        return res->second;
    }
    throw std::runtime_error("Interface requested on out-of-scope contract.");
}

// -------------------------------------------------------------------------- //

ContractDependance::FunctionSet const& ContractDependance::get_function_roi(
    FunctionDefinition const* _func
) const
{
    auto res = m_callreach.find(_func);
    if (res != m_callreach.end())
    {
        return res->second;
    }
    throw std::runtime_error("Function ROI requested on out-of-scope method.");
}

// -------------------------------------------------------------------------- //

ContractDependance::VarSet const& ContractDependance::get_map_roi(
    FunctionDefinition const* _func
) const
{
    auto res = m_mapreach.find(_func);
    if (res != m_mapreach.end())
    {
        return res->second;
    }
    throw std::runtime_error("Map ROI requested on out-of-scope method.");
}

// -------------------------------------------------------------------------- //

}
}
}
