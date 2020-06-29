#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>

#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
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

ContractRvAnalyzer::ContractRvAnalyzer(
    shared_ptr<AllocationGraph const> _allocation_graph,
    FunctionDefinition const& _func
): m_src(nullptr), m_allocation_graph(_allocation_graph)
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

set<ContractRvAnalyzer::Key> ContractRvAnalyzer::dependencies() const
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
                m_procedural_calls.insert(make_pair(m_src, &call.decl()));
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
                m_procedural_calls.insert(make_pair(user, match));
            }
        }
        else if (call.is_in_library())
        {
            m_procedural_calls.insert(make_pair(nullptr, &call.decl()));
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

ContractRvLookup::ContractRvLookup(
    FlatModel const& _model, shared_ptr<AllocationGraph const> _allocation_graph
): m_allocation_graph(_allocation_graph)
{
    // First, the contract methods used by the model are recorded.
    set<Key> seen;
    for (auto contract : _model.view())
    {
        for (auto func : contract->interface())
        {
            if (check_method_rv(*func))
            {
                seen.insert(record(*contract, *func));
            }
        }
        for (auto func : contract->internals())
        {
            if (check_method_rv(*func))
            {
                seen.insert(record(*contract, *func));
            }
        }
    }
    list<Key> pending(seen.begin(), seen.end());

    // Next, the required libraries are analyzed and added.
    for (auto key : pending)
    {
        auto record = registry[key];
        for (auto dep : record->dependencies())
        {
            if (dep.first == nullptr)
            {
                if (seen.insert(dep).second)
                {
                    registry[dep] = make_shared<ContractRvAnalyzer>(
                        m_allocation_graph, *key.second
                    );
                    pending.push_back(dep);
                }
            }
        }
    }
}

ContractRvLookup::Key ContractRvLookup::record(
    FlatContract const& _src, FunctionDefinition const& _func
)
{
    auto key = make_pair(_src.raw(), &_func);
    registry[key] = make_shared<ContractRvAnalyzer>(
        *_src.raw(), m_allocation_graph, _func
    );
    return key;
}

bool ContractRvLookup::check_method_rv(FunctionDefinition const& _func)
{
    auto const& params = _func.returnParameters();
    if (params.size() != 1)
    {
        return false;
    }

    if (params[0]->type()->category() != Type::Category::Contract)
    {
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------- //

ContractExpressionAnalyzer::ContractExpressionAnalyzer(
    FlatModel const& _model, shared_ptr<AllocationGraph const> _allocation_graph
): m_allocation_graph(_allocation_graph)
{
    ContractRvLookup lookup(_model, _allocation_graph);
    for (auto entry : lookup.registry)
    {
        auto const& src = (*entry.second);
        auto const& src_deps = src.dependencies();

        list<ContractRvLookup::Key> pending(src_deps.begin(), src_deps.end());
        set<ContractRvLookup::Key> seen;
        seen.insert(entry.first);

        // Aggregates all internals and externals across all dependencies.
        set<ContractDefinition const*> internal = src.internals();
        set<ContractDefinition const*> external = src.externals();
        for (auto key : pending)
        {
            if (seen.insert(key).second)
            {
                auto record = (*lookup.registry[key]);
                auto internal_ext = record.internals();
                auto external_ext = record.externals();
                internal.insert(internal_ext.begin(), internal_ext.end());
                external.insert(external_ext.begin(), external_ext.end());

                for (auto dep : record.dependencies())
                {
                    if (seen.insert(dep).second)
                    {
                        pending.push_back(dep);
                    }
                }
            }
        }

        // Ensures that this function satisfies our requirements.
        if (!external.empty() && internal.size() != 1)
        {
            throw runtime_error("Unable to resolve contract rv type.");
        }
        m_rv_types[entry.first] = (*internal.begin());
    }
}

ContractDefinition const& ContractExpressionAnalyzer::resolve(
    Expression const& _expr, ContractDefinition const* _ctx
) const
{
    // Ensures the type matches the request.
    auto type = _expr.annotation().type;
    if (type->category() != Type::Category::Contract)
    {
        throw runtime_error("Resolve must be called on a contract typed expr.");
    }

    // Functions are handled differently.
    ExpressionCleaner cleaner(_expr);
    if (auto call = dynamic_cast<FunctionCall const*>(&cleaner.clean()))
    {
        FunctionCallAnalyzer calldata(*call);

        // Determines the key for this call.
        auto key = make_pair(_ctx, &calldata.decl());
        if (calldata.is_in_library())
        {
            key.first = nullptr;
        }
        else if (calldata.context() && !calldata.context_is_this())
        {
            key.first = (&resolve(*calldata.context(), _ctx));
        }

        // Resolves the key.
        auto match =  m_rv_types.find(key);
        if (match == m_rv_types.end())
        {
            throw runtime_error("Unable to resolve rv type of call");
        }
        return (*match->second);
    }
    else
    {
        auto decl = expr_to_decl(_expr);
        if (!decl)
        {
            throw runtime_error("Unable to resolve expression to decl.");
        }
        return m_allocation_graph->specialize(*decl);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
