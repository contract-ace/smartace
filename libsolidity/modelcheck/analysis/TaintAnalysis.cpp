#include <libsolidity/modelcheck/analysis/TaintAnalysis.h>

#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/General.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

TaintDestination::TaintDestination(Expression const& _expr)
{
    _expr.accept(*this);
}

VariableDeclaration const& TaintDestination::extract()
{
    if (!m_dest)
    {
        throw runtime_error("TaintDestination: Failed to find destination.");
    }
    else
    {
        return (*m_dest);
    }
}

bool TaintDestination::visit(IndexAccess const& _node)
{
    _node.baseExpression().accept(*this);
    return false;
}

bool TaintDestination::visit(MemberAccess const& _node)
{
    if (auto const* ref = _node.annotation().referencedDeclaration)
    {
        update_dest(ref);
    }
    else
    {
        throw runtime_error("TaintDestination: Expected member declaration.");
    }
    return false;
}

bool TaintDestination::visit(Identifier const& _node)
{
    if (auto const* ref = _node.annotation().referencedDeclaration)
    {
        update_dest(ref);
    }
    return false;
}

void TaintDestination::update_dest(Declaration const* _ref)
{
    // Checks that set happens once.
    if (m_dest != nullptr)
    {
        throw runtime_error("TaintDestination: m_dest set twice.");
    }

    // Extraction.
    if (auto decl = dynamic_cast<VariableDeclaration const*>(_ref))
    {
        m_dest = decl;
    }
    else
    {
        throw runtime_error("TaintDestination: Failed to extract definition.");
    }
}

// -------------------------------------------------------------------------- //

TaintAnalysis::TaintAnalysis(size_t _sources)
 : m_sources(_sources)
{
    m_default_taint.resize(_sources, false);
}

void TaintAnalysis::taint(VariableDeclaration const& _decl, size_t _i)
{
    // Ensures taint is in bounds.
    if (_i >= m_sources)
    {
        throw runtime_error("Invalid taint value.");
    }

    // Adds entry.
    auto & taint = m_taint[&_decl];
    if (taint.size() != m_sources)
    {
        taint.resize(m_sources, false);
    }

    // Updates value and records change.
    if (!taint[_i])
    {
        m_changed = true;
        taint[_i] = true;
    }
}

void TaintAnalysis::run(FunctionDefinition const& _decl)
{
    m_taintee = nullptr;
    while (m_changed)
    {
        m_changed = false;
        _decl.body().accept(*this);
    }
}

vector<bool> const&
TaintAnalysis::taint_for(VariableDeclaration const& _decl) const
{
    auto res = m_taint.find(&_decl);
    if (res != m_taint.end())
    {
        return res->second;
    }
    else
    {
        return m_default_taint;
    }
}

size_t TaintAnalysis::source_count() const
{
    return m_sources;
}

bool TaintAnalysis::visit(VariableDeclarationStatement const& _node)
{
    if (_node.declarations().size() > 1)
    {
        // TODO: this is currently unsupported in Block_general.
		throw runtime_error("Multi-decls stmt unsupoorted in Block_general.");
    }
	else if (!_node.declarations().empty())
	{
        if (auto const* val = _node.initialValue())
        {
            auto const* decl = _node.declarations()[0].get();
            ScopedSwap<VariableDeclaration const*> scope(m_taintee, decl);
            val->accept(*this);
        }
    }
    return false;
}

bool TaintAnalysis::visit(Assignment const& _node)
{
    // Cleans lhs to check for tuples.
    ExpressionCleaner l_cleaner(_node.leftHandSide());
    auto const& lexpr = l_cleaner.clean();

    // Detects tuple assignment.
    if (auto lhs = dynamic_cast<TupleExpression const*>(&lexpr))
    {
        // Cleans rhs to check for tuple assignment type.
        ExpressionCleaner r_cleaner(_node.rightHandSide());
        auto const& rexpr = r_cleaner.clean();

        // Classifies tuple assignment.
        if (dynamic_cast<FunctionCall const*>(&rexpr))
        {
            for (auto const& comp : lhs->components())
            {
                auto &dest = TaintDestination(*comp.get()).extract();
                ScopedSwap<VariableDeclaration const*> scope(m_taintee, &dest);
                propogate_unknown();
            }
        }
        else if (auto rhs = dynamic_cast<TupleExpression const*>(&rexpr))
        {
            auto const& lcomps = lhs->components();
            auto const& rcomps = rhs->components();
            for (size_t i = 0; i < rcomps.size(); ++i)
            {
                auto &dest = TaintDestination(*lcomps[i].get()).extract();
                ScopedSwap<VariableDeclaration const*> scope(m_taintee, &dest);
                rcomps[i]->accept(*this);
            }
        }
	    else
	    {
		    throw runtime_error("TaintAnalysis: Unexpected LHS tuple.");
	    }
    }
    else
    {
        auto &dest = TaintDestination(lexpr).extract();
        ScopedSwap<VariableDeclaration const*> scope(m_taintee, &dest);
        _node.rightHandSide().accept(*this);
    }
    return false;
}

bool TaintAnalysis::visit(FunctionCall const&)
{
    propogate_unknown();
    return false;
}

bool TaintAnalysis::visit(MemberAccess const& _node)
{
    if (auto const* ref = _node.annotation().referencedDeclaration)
    {
        propogate(ref);
    }
    else
    {
        auto cat = _node.expression().annotation().type->category();
        if (cat != Type::Category::Magic && cat != Type::Category::TypeType)
        {
            cout << get_ast_string(&_node) << endl;
            throw runtime_error("TaintAnalysis: Expected member declaration.");
        }
    }
    return false;
}

bool TaintAnalysis::visit(Identifier const& _node)
{
    if (auto const* ref = _node.annotation().referencedDeclaration)
    {
        propogate(ref);
    }
    return false;
}

void TaintAnalysis::propogate(Declaration const* _ref)
{
    if (m_taintee)
    {
        if (auto decl = dynamic_cast<VariableDeclaration const*>(_ref))
        {
            auto res = m_taint.find(decl);
            if (res != m_taint.end())
            {
                for (size_t i = 0; i < m_sources; ++i)
                {
                    if (res->second[i])
                    {
                        taint(*m_taintee, i);
                    }
                }
            }
        }
    }
}

void TaintAnalysis::propogate_unknown()
{
    if (m_taintee)
    {
        for (size_t i = 0; i < m_sources; ++i)
        {
            taint(*m_taintee, i);
        }
    }
}

// -------------------------------------------------------------------------- //

ClientTaintPass::ClientTaintPass(FunctionDefinition const& _func)
{
    // Computes number of potentia clients.
    size_t potential_clients = 0;
    for (auto param : _func.parameters())
    {
        if (param->type()->category() == Type::Category::Address)
        {
            potential_clients += 1;
        }
    }

    // Aborts now if there are no clients to analyze.
    if (potential_clients == 0)
    {
        return;
    }

    // Generates tainted sources.
    m_reached_sinks.resize(potential_clients, false);
    m_taint_data = make_shared<TaintAnalysis>(potential_clients);
    size_t taint_id = 0;
    for (auto param : _func.parameters())
    {
        if (param->type()->category() == Type::Category::Address)
        {
            m_taint_data->taint(*param.get(), taint_id);
            taint_id += 1;
        }
    }

    // Computes taint results.
    m_taint_data->run(_func);

    // Since the analysis is intraprocedural modifier calls are also sinks.
    for (auto modifier : _func.modifiers())
    {
        if (auto const* arguments = modifier->arguments())
        {
            for (auto const& arg : (*arguments))
            {
                ScopedSwap<bool> scope(m_in_sink, true);
                arg->accept(*this);
            }
        }
    }

    // Computes analysis results for index accesses and expression comparisons.
    _func.accept(*this);
}

vector<bool> const& ClientTaintPass::extract() const
{
    return m_reached_sinks;
}

bool ClientTaintPass::visit(Return const& _node)
{
    ScopedSwap<bool> scope(m_in_sink, true);
    if (auto const* expr = _node.expression())
    {
        expr->accept(*this);
    }
    return false;
}

bool ClientTaintPass::visit(FunctionCall const& _node)
{
    {
        ScopedSwap<bool> scope(m_in_sink, true);
        for (auto arg : _node.arguments())
        {
            arg->accept(*this);
        }
    }
    _node.expression().accept(*this);
    return false;
}

bool ClientTaintPass::visit(UnaryOperation const& _node)
{
    ScopedSwap<bool> scope(m_in_sink, true);
    _node.subExpression().accept(*this);
    return false;
}

bool ClientTaintPass::visit(BinaryOperation const& _node)
{
    ScopedSwap<bool> scope(m_in_sink, true);
    _node.leftExpression().accept(*this);
    _node.rightExpression().accept(*this);
    return false;
}

bool ClientTaintPass::visit(IndexAccess const& _node)
{
    if (auto const* expr = _node.indexExpression())
    {
        ScopedSwap<bool> scope(m_in_sink, true);
        expr->accept(*this);
    }
    _node.baseExpression().accept(*this);
    return false;
}

bool ClientTaintPass::visit(MemberAccess const& _node)
{
    // Processes declaration.
    if (auto const* ref = _node.annotation().referencedDeclaration)
    {
        process_declaration(ref);
    }

    // Resets using flag, and continues down access chain (eg. for fn calls).
    {
        ScopedSwap<bool> scope(m_in_sink, false);
        _node.expression().accept(*this);
    }

    return false;
}

bool ClientTaintPass::visit(Identifier const& _node)
{
    if (auto const* ref = _node.annotation().referencedDeclaration)
    {
        process_declaration(ref);
    }
    return false;
}

void ClientTaintPass::process_declaration(Declaration const* _ref)
{
    if (auto decl = dynamic_cast<VariableDeclaration const*>(_ref))
    {
        // If this is a sink, extract taint.
        // Recall that state variables are sinks.
        auto const& taint = m_taint_data->taint_for(*decl);
        bool is_state = (!decl->isLocalVariable() || decl->isReturnParameter());
        if (m_in_sink || is_state)
        {
            for (size_t i = 0; i < taint.size(); ++i)
            {
                if (taint[i])
                {
                    m_reached_sinks[i] = true;
                }
            }
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
