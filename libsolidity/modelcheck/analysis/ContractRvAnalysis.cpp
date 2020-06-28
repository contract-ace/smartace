#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/AST.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ContractRvAnalyzer::ContractRvAnalyzer(
    ContractDefinition const& _src,
    shared_ptr<AllocationGraph const> _allocation_graph,
    FunctionDefinition const& _func
): m_src(&_src), m_allocation_graph(_allocation_graph)
{
    _func.body().accept(*this);
}

set<ContractDefinition const*> ContractRvAnalyzer::internals() const
{
    return m_internal_refs;
}

set<ContractDefinition const*> ContractRvAnalyzer::externals() const
{
    return m_external_refs;
}

set<FunctionDefinition const*> ContractRvAnalyzer::dependencies() const
{
    return m_procedural_calls;
}

bool ContractRvAnalyzer::visit(IfStatement const& _node)
{
    _node.trueStatement().accept(*this);
    if (auto stmt = _node.falseStatement()) stmt->accept(*this);
    return false;
}

bool ContractRvAnalyzer::visit(WhileStatement const& _node)
{
    _node.body().accept(*this);
    return false;
}

bool ContractRvAnalyzer::visit(ForStatement const& _node)
{
    _node.body().accept(*this);
    return false;
}

bool ContractRvAnalyzer::visit(Conditional const& _node)
{
    _node.trueExpression().accept(*this);
    _node.falseExpression().accept(*this);
    return false;
}

bool ContractRvAnalyzer::visit(Return const& _node)
{
    if (auto rv = _node.expression())
    {
        ScopedSwap<bool> guard(m_is_in_return, true);
        rv->accept(*this);
    }
    return false;
}

bool ContractRvAnalyzer::visit(Assignment const& _node)
{
    bool is_rv = false;
    if (auto decl = expr_to_decl(_node.leftHandSide()))
    {
        is_rv = decl->isReturnParameter();
    }

    ScopedSwap<bool> guard(m_is_in_return, is_rv);
    _node.rightHandSide().accept(*this);
    return false;
}

bool ContractRvAnalyzer::visit(FunctionCall const& _node)
{
    if (!m_is_in_return) return false;

	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND == FunctionCallKind::FunctionCall)
    {
        FunctionCallAnalyzer call(_node);
        if (call.classify() == FunctionCallAnalyzer::CallGroup::Method)
        {
            if (call.is_super())
            {
                m_procedural_calls.insert(&call.decl());
            }
            else
            {
                auto user = m_src;
 
                if (dynamic_cast<FunctionCall const*>(call.context()))
                {
                    // TODO(scottwe): find a solution for this case.
                    // TODO(scottwe): one case is no child classes...
                    throw runtime_error("Circular contract rv dependence.");
                }
                else if (dynamic_cast<IndexAccess const*>(call.context()))
                {
                    // TODO(scottwe): find a solution for this case.
                    throw runtime_error("Index not allowed for contract rv");
                }
                else if (call.context())
                {
                    auto decl = expr_to_decl(*call.context());
                    if (!decl->isStateVariable())
                    {
                        throw runtime_error("State var needed for contract rv");
                    }
                    user = (&m_allocation_graph->specialize(*decl));
                }

                string const& name = call.decl().name();
                auto match = find_named_match<FunctionDefinition>(user, name);
                m_procedural_calls.insert(match);
            }
        }
        else if (call.is_in_library())
        {
            m_procedural_calls.insert(&call.decl());
        }
        else if (call.classify() == FunctionCallAnalyzer::CallGroup::Delegate)
        {
            throw runtime_error("Delegate calls are not allowed.");
        }
    }
    else if (KIND == FunctionCallKind::TypeConversion)
    {
        auto type = _node.annotation().type;
        auto contract = dynamic_cast<ContractType const*>(type);
        m_external_refs.insert(&contract->contractDefinition());
    }

    return false;
}

bool ContractRvAnalyzer::visit(MemberAccess const& _node)
{
    record_ref(_node);
    return false;
}

bool ContractRvAnalyzer::visit(IndexAccess const&)
{
    if (m_is_in_return)
    {
        throw runtime_error("Reference to mapping of contracts unsupported.");
    }
    return false;
}

bool ContractRvAnalyzer::visit(Identifier const& _node)
{
    record_ref(_node);
    return false;
}

void ContractRvAnalyzer::record_ref(Expression const& _ref)
{
    if (m_is_in_return)
    {
        if (auto decl = expr_to_decl(_ref))
        {
            m_internal_refs.insert(&m_allocation_graph->specialize(*decl));
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
