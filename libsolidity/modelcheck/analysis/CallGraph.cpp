#include <libsolidity/modelcheck/analysis/CallGraph.h>

#include <libsolidity/modelcheck/analysis/ContractRvAnalysis.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/Function.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

shared_ptr<FlatContract> devirtualize(
    shared_ptr<FlatContract> _scope,
    ContractExpressionAnalyzer const& _expr_resolver,
    FlatModel const& _model,
    FunctionCallAnalyzer const& _func
)
{
    shared_ptr<FlatContract> scope;

    // Best effort to devirtualize.
    auto ctx = _func.context();
    if (_func.is_in_library())
    {
        scope = _scope;
    }
    else if (_func.is_super())
    {
        auto new_scope = _func.decl().scope();
        auto context = dynamic_cast<ContractDefinition const*>(new_scope);
        scope = _model.get(*context);
    }
    else if (ctx && !_func.context_is_this())
    {
        auto const& context = _expr_resolver.resolve(*ctx, _scope->raw());
        scope = _model.get(context);
    }
    else
    {
        scope = _scope;
    }

    // Failed to devirtualize.
    if (!scope)
    {
        throw runtime_error("Failed to devirtualize external call.");
    }

    return scope;
}

// -------------------------------------------------------------------------- //

CallGraphBuilder::CallGraphBuilder(
    shared_ptr<ContractExpressionAnalyzer const> _expr_resolver
): m_expr_resolver(_expr_resolver) {}

shared_ptr<CallGraphBuilder::Graph>
    CallGraphBuilder::build(shared_ptr<FlatModel const> _model)
{
    ScopedSwap<shared_ptr<FlatModel const>> scope(m_model, _model);

    m_graph = make_shared<CallGraphBuilder::Graph>();
    for (auto contract : _model->view())
    {
        m_scope.push_back(contract);

        // All public interfaces all explored in the model.
        for (auto func : contract->interface())
        {
            func->accept(*this);
        }

        // All constructors are called at least once, implicitly.
        for (auto ctor : contract->constructors())
        {
            ctor->accept(*this);
        }

        // Fallback methods can be called directly.
        if (auto fallback = contract->fallback())
        {
            fallback->accept(*this);
        }

        m_scope.pop_back();
    }
    return std::move(m_graph);
}

bool CallGraphBuilder::visit(FunctionDefinition const& _node)
{
    // Caches labels.
    auto labels = m_labels;

    // Processes vertex if it is new.
    if (!m_graph->has_vertex(&_node))
    {
        m_graph->add_vertex(&_node);

        m_stack.push_back(&_node);
        _node.body().accept(*this);
        m_stack.pop_back();
    }

    // Adds edge if this is not a root.
    if (!m_stack.empty())
    {
        m_graph->add_edge(m_stack.back(), &_node);
        for (auto label : labels)
        {
            m_graph->label_edge(m_stack.back(), &_node, label);
        }
    }

    return false;
}

void CallGraphBuilder::endVisit(FunctionCall const& _node)
{
    // If the type isn't a function this isn't a super call.
	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND != FunctionCallKind::FunctionCall) return;

    // If the call is low-level, it isn't analyzed at this level.
	FunctionCallAnalyzer call(_node);
    if (call.is_low_level()) return;

    // Otherwise checks if this is a contract method call.
    shared_ptr<FlatContract> scope;
    FunctionDefinition const* resolution = nullptr;
    m_labels.clear();
    if (call.is_in_library())
    {
        m_labels = { CallTypes::Library };
        resolution = (&call.decl());
    }
    else if (call.classify() == FunctionCallAnalyzer::CallGroup::Method)
    {
        // Sets the context label. Note context, library, and super are mutually
        // exclusive so this is fine.
        if (call.context())
        {
            m_labels = { CallTypes::External };
        }

        // Resolves the function using the scope of this call.
        scope = devirtualize(m_scope.back(), *m_expr_resolver, *m_model, call);
        resolution = (&scope->resolve(call.decl()));

        // If this is a super call, the scope really shouldn't change.
        if (call.is_super())
        {
            m_labels = { CallTypes::Super };
            scope = m_scope.back();
        }
    }
    else if (call.classify() == FunctionCallAnalyzer::CallGroup::Constructor)
    {
        auto raw_type = _node.annotation().type;
        auto contract = dynamic_cast<ContractType const*>(raw_type);
        scope = m_model->get(contract->contractDefinition());
        if (!scope->constructors().empty())
        {
            m_labels = { CallTypes::Alloc };
            resolution = scope->constructors().front();
        }
    }
    else if (call.classify() == FunctionCallAnalyzer::CallGroup::Delegate)
    {
        throw runtime_error("Delegate calls are unsupported.");
    }

    // Follows the call, if it was resolved
    if (resolution)
    {
        m_scope.push_back(scope);
        resolution->accept(*this);
        m_scope.pop_back();
    }
}

// -------------------------------------------------------------------------- //

CallGraph::CallGraph(
    shared_ptr<ContractExpressionAnalyzer const> _expr_resolver,
    shared_ptr<FlatModel const> _model
): m_graph(CallGraphBuilder(_expr_resolver).build(_model)) {}

CallGraph::CodeSet CallGraph::executed_code() const
{
    return m_graph->vertices();
}

CallGraph::CodeSet CallGraph::internals(FlatContract const& _scope) const
{
    CodeSet methods;

    // Computes list of all interfaces, including "special" methods.
    CodeList functions = _scope.interface();
    if (auto fallback = _scope.fallback())
    {
        functions.push_back(fallback);
    }
    for (auto ctor : _scope.constructors())
    {
        functions.push_back(ctor);
    }

    // TODO: precompute.
    set<FunctionDefinition const*> visited;
    for (auto itr = functions.begin(); itr != functions.end(); ++itr)
    {
        FunctionDefinition const* func = (*itr);
        if (!visited.insert(func).second) continue;

        if (!func->functionType(false)) methods.insert(func);

        for (auto succ : m_graph->neighbours(func))
        {
            auto labels = m_graph->label_of(func, succ);
            if (labels.find(CallTypes::External) == labels.end())
            {
                if (labels.find(CallTypes::Library) == labels.end())
                {
                    functions.push_back(succ);
                }
            }
        }
    }

    return methods;
}

CallGraph::CodeSet CallGraph::super_calls(
    FlatContract const& _scope, FunctionDefinition const& _call
) const
{
    CodeSet chain;

    CodeList functions;
    if (_call.functionType(false))
    {
        functions = _scope.interface();
    }
    else
    {
        functions = _scope.internals();
    }

    // TODO: precompute.
    set<FunctionDefinition const*> visited;
    for (auto itr = functions.begin(); itr != functions.end(); ++itr)
    {
        FunctionDefinition const* func = (*itr);
        if (!visited.insert(func).second) continue;

        if (collid(_call, *func)) chain.insert(func);

        for (auto succ : m_graph->neighbours(func))
        {
            auto labels = m_graph->label_of(func, succ);
            if (labels.find(CallTypes::External) == labels.end())
            {
                if (labels.find(CallTypes::Library) == labels.end())
                {
                    functions.push_back(succ);
                }
            }
        }
    }

    return chain;
}

// -------------------------------------------------------------------------- //

}
}
}
