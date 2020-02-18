/**
 * @date 2020
 * Provides utilities for generating an abstract address space for map indices.
 */

#include <libsolidity/modelcheck/analysis/MapIndex.h>

#include <libsolidity/modelcheck/utils/General.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MapIndexSummary::MapIndexSummary(ContractDefinition const& _src)
    : m_is_address_cast(false)
{
    // Summarizes state address variables.
    for (auto const* var : _src.stateVariables())
    {
        if (var->type()->category() == Type::Category::Address)
        {
            // TODO(scottwe): implement.
            // TODO(scottwe): catch literals.
            throw std::runtime_error("Address variables unsupported.");
        }
    }

    // Summarizes functions.
    for (auto const* func : _src.definedFunctions())
    {
        ScopedSwap<FunctionDefinition const*> scope(m_context, func);
        func->accept(*this);
    }
}

MapIndexSummary::ViolationGroup const& MapIndexSummary::violations() const
{
    return m_violations;
}

std::set<dev::u256> const& MapIndexSummary::literals() const
{
    return m_literals;
}

// -------------------------------------------------------------------------- //

bool MapIndexSummary::visit(VariableDeclaration const& _node)
{
    if (_node.value() == nullptr)
    {
        if (_node.type()->category() == Type::Category::Address)
        {
            m_literals.insert(dev::u256(0));
        }
    }
    return true;
}

bool MapIndexSummary::visit(UnaryOperation const& _node)
{
    if (m_is_address_cast)
    {
        m_violations.emplace_front();
        m_violations.front().type = ViolationType::Mutate;
        m_violations.front().context = m_context;
        m_violations.front().site = (&_node);
        return false;
    }
    return true;
    
}

bool MapIndexSummary::visit(BinaryOperation const& _node)
{
    if (m_is_address_cast)
    {
        m_violations.emplace_front();
        m_violations.front().type = ViolationType::Mutate;
        m_violations.front().context = m_context;
        m_violations.front().site = (&_node);
        return false;
    }

    auto const& lhs = _node.leftExpression();
    if (lhs.annotation().type->category() == Type::Category::Address)
    {
        auto const TOK = _node.getOperator();
        if (TOK != Token::Equal && TOK != Token::NotEqual)
        {
            m_violations.emplace_front();
            m_violations.front().type = ViolationType::Compare;
            m_violations.front().context = m_context;
            m_violations.front().site = (&_node);
            return false;
        }
    }

    return true;
}

bool MapIndexSummary::visit(FunctionCall const& _node)
{
    if (m_is_address_cast)
    {
        m_violations.emplace_front();
        m_violations.front().type = ViolationType::Mutate;
        m_violations.front().context = m_context;
        m_violations.front().site = (&_node);
        return false;
    }
    else if (_node.annotation().kind == FunctionCallKind::TypeConversion)
    {
        auto base = _node.arguments()[0];
        if (_node.annotation().type->category() == Type::Category::Address)
        {
            ScopedSwap<bool> scope(m_is_address_cast, true);
            base->accept(*this);
        }
        else if (base->annotation().type->category() == Type::Category::Address)
        {
            m_violations.emplace_front();
            m_violations.front().type = ViolationType::Cast;
            m_violations.front().context = m_context;
            m_violations.front().site = (&_node);
        }
        else
        {
            base->accept(*this);
        }
    }
    else
    {
        for (auto arg : _node.arguments()) arg->accept(*this);
    }
    return false;
}

bool MapIndexSummary::visit(MemberAccess const& _node)
{
    if (m_is_address_cast)
    {
        m_violations.emplace_front();
        m_violations.front().type = ViolationType::Mutate;
        m_violations.front().context = m_context;
        m_violations.front().site = (&_node);
        return false;
    }
    return true;
}

bool MapIndexSummary::visit(Identifier const& _node)
{
    if (m_is_address_cast)
    {
        m_violations.emplace_front();
        m_violations.front().type = ViolationType::Mutate;
        m_violations.front().context = m_context;
        m_violations.front().site = (&_node);
        return false;
    }
    return true;
}

bool MapIndexSummary::visit(Literal const& _node)
{
    if (m_is_address_cast)
    {
        m_literals.insert(_node.annotation().type->literalValue(&_node));
    }
    return true;
}

// -------------------------------------------------------------------------- //

}
}
}
