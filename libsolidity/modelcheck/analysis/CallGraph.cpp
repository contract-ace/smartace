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
        m_locations.emplace_back(Location{ contract, contract });

        // All public interfaces all explored in the model.
        for (auto func : contract->interface())
        {
            auto scope = dynamic_cast<ContractDefinition const*>(func->scope());
            m_locations.emplace_back();
            m_locations.back().entry = contract;
            m_locations.back().scope = m_model->get(*scope);

            func->accept(*this);

            m_locations.pop_back();
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

        m_locations.pop_back();
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
    Location loc;
    FunctionDefinition const* resolution = nullptr;
    m_labels.clear();
    if (call.is_in_library())
    {
        m_labels = { CallTypes::Library };
        resolution = (&call.method_decl());
    }
    else if (call.classify() == FunctionCallAnalyzer::CallGroup::Method)
    {
        if (!call.is_getter())
        {
            // Sets the context label. Note context, library, and super are
            // mutually exclusive so this is fine.
            if (call.context())
            {
                m_labels = { CallTypes::External };
            }
            if (call.is_super())
            {
                m_labels = { CallTypes::Super };
            }

            // Resolves the function using the scope of this call.
            loc = devirtualize(call);
            resolution = (&loc.scope->resolve(call.method_decl()));
        }
    }
    else if (call.classify() == FunctionCallAnalyzer::CallGroup::Constructor)
    {
        auto raw_type = _node.annotation().type;
        auto contract_type = dynamic_cast<ContractType const*>(raw_type);
        loc.entry = m_model->get(contract_type->contractDefinition());
        loc.scope = loc.entry;
        if (!loc.scope->constructors().empty())
        {
            m_labels = { CallTypes::Alloc };
            resolution = loc.scope->constructors().front();
        }
    }
    else if (call.classify() == FunctionCallAnalyzer::CallGroup::Delegate)
    {
        throw runtime_error("Delegate calls are unsupported.");
    }

    // Follows the call, if it was resolved
    if (resolution)
    {
        m_locations.push_back(std::move(loc));
        resolution->accept(*this);
        m_locations.pop_back();
    }
}

CallGraphBuilder::Location
    CallGraphBuilder::devirtualize(FunctionCallAnalyzer const& _func) const
{
    // The library case bypasses most checks.
    Location new_loc;
    Location const& old_loc = m_locations.back();
    if (_func.is_in_library())
    {
        new_loc.entry = old_loc.entry;
        new_loc.scope = old_loc.entry;
        return new_loc;
    }

    // Best effort to follow super calls and external calls.
    auto ctx = _func.context();
    if (_func.is_super())
    {
        // The super call must be resolved manually due to diamond inheritance.
        auto parent = m_model->next_ancestor(*old_loc.entry, *old_loc.scope);
        auto const& superfunc = parent->resolve(_func.method_decl());
        auto context = dynamic_cast<ContractDefinition const*>(superfunc.scope());
        new_loc.entry = old_loc.entry;
        new_loc.scope = m_model->get(*context);
    }
    else
    {
        // Determines if the entry context changes..
        auto context = old_loc.entry;
        if (ctx && !_func.context_is_this())
        {
            context = m_expr_resolver->resolve(*ctx, context);
        }

        // If this is not a super call, then the "scope" resets.
        auto const& actual = context->resolve(_func.method_decl());
        auto scope = dynamic_cast<ContractDefinition const*>(actual.scope());
        new_loc.entry = context;
        new_loc.scope = m_model->get(*scope);
    }

    // Failed to devirtualize.
    if (!new_loc.entry || !new_loc.scope)
    {
        throw runtime_error("Failed to devirtualize external call.");
    }

    return new_loc;
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
    for (auto func : functions)
    {
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
    for (auto func : functions)
    {
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
