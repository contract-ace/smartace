/**
 * @date 2020
 * Provides utilities for generating an abstract address space for map indices.
 */

#include <libsolidity/modelcheck/analysis/MapIndex.h>

#include <libsolidity/modelcheck/utils/General.h>

#include <stack>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MapIndexSummary::MapIndexSummary(uint64_t clients, uint64_t contracts)
    : m_client_reps(clients)
    , m_contract_reps(contracts)
    , m_max_inteference(0)
    , m_in_first_pass(false)
    , m_is_address_cast(false)
    , m_uses_contract_address(false)
    , m_context(nullptr)
{
}

void MapIndexSummary::extract_literals(ContractDefinition const& _src)
{
    // Summarizes address state variable initialization.
    for (auto const* var : _src.stateVariables())
    {
        if (var->type()->category() == Type::Category::Address)
        {
            var->accept(*this);
        }
        else if (var->type()->category() == Type::Category::Mapping)
        {
            auto const* map = dynamic_cast<MappingType const*>(var->type());
            while (map)
            {
                if (map->keyType()->category() != Type::Category::Address)
                {
                    // TODO: new error types.
                    throw runtime_error("Mappings must be address-based.");
                }
                map = dynamic_cast<MappingType const*>(map->valueType());
            }
        }
    }

    // Summarizes functions.
    for (auto const* func : _src.definedFunctions())
    {
        ScopedSwap<CallableDeclaration const*> scope(m_context, func);
        func->body().accept(*this);
    }
    for (auto const* mod : _src.functionModifiers())
    {
        ScopedSwap<CallableDeclaration const*> scope(m_context, mod);
        mod->body().accept(*this);
    }
}

void MapIndexSummary::compute_interference(ContractDefinition const& _src)
{
    // Summarizes state address variables.
    uint64_t address_var_count = 0;
    for (auto const* var : _src.stateVariables())
    {
        auto varcat = var->type()->category();
        if (varcat == Type::Category::Address)
        {
            address_var_count += 1;
        }
        else if (varcat == Type::Category::Mapping)
        {
            uint64_t entries = representative_count();

            auto const* mapping = dynamic_cast<MappingType const*>(var->type());
            while (mapping->valueType()->category() == Type::Category::Mapping)
            {
                entries *= representative_count();
                mapping = dynamic_cast<MappingType const*>(mapping->valueType());
            }

            if (mapping->valueType()->category() == Type::Category::Address)
            {
                address_var_count += entries;
            }
        }
        else if (varcat == Type::Category::Struct)
        {
            auto const* type = dynamic_cast<StructType const*>(var->type());

            stack<StructDefinition const*> frontier;
            frontier.push(&type->structDefinition());

            while (!frontier.empty())
            {
                StructDefinition const* level = frontier.top();
                frontier.pop();

                for (auto subvar : level->members())
                {
                    auto subvarcat = subvar->type()->category();
                    if (subvarcat == Type::Category::Address)
                    {
                        address_var_count += 1;
                    }
                    else if (subvarcat == Type::Category::Struct)
                    {
                        type = dynamic_cast<StructType const*>(subvar->type());
                        frontier.push(&type->structDefinition());
                    }
                }
            }
        }
    }

    // Summarizes functions.
    for (auto const* func : _src.definedFunctions())
    {
        // All state addresses, along with the sender may be interference.
        if (func->isPublic())
        {
            uint64_t potential_interference = address_var_count + 1;
            for (auto var : func->parameters())
            {
                if (var->type()->category() == Type::Category::Address)
                {
                    ++potential_interference;
                }
            }

            if (potential_interference > m_max_inteference)
            {
                m_max_inteference = potential_interference;
            }
        }
    }
}

MapIndexSummary::ViolationGroup const& MapIndexSummary::violations() const
{
    return m_violations;
}

set<dev::u256> const& MapIndexSummary::literals() const
{
    return m_literals;
}

uint64_t MapIndexSummary::representative_count() const
{
    return client_count() + m_contract_reps + m_literals.size();
}

uint64_t MapIndexSummary::client_count() const
{
    return m_client_reps;
}

uint64_t MapIndexSummary::max_interference() const
{
    return m_max_inteference;
}

uint64_t MapIndexSummary::size() const
{
    return representative_count() + max_interference();
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
    // TODO(scottwe): evaluate constant expression at analysis time. I think
    //                the compiler already does this so we could copy that. This
    //                is worth adding to expression evaluations as well...
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
    // TODO(scottwe): see todo in `visit(UnaryOperation const&)`.

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
    // TODO(scottwe): see todo in `visit(UnaryOperation const&)`.

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
    // TODO(scottwe): see todo in `visit(UnaryOperation const&)`.

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
    // TODO(scottwe): see todo in `visit(UnaryOperation const&)`.

    if (m_is_address_cast)
    {
        if (_node.name() == "this")
        {
            m_uses_contract_address = true;
        }
        else if (_node.annotation().type->category() != Type::Category::Contract)
        {
            m_violations.emplace_front();
            m_violations.front().type = ViolationType::Mutate;
            m_violations.front().context = m_context;
            m_violations.front().site = (&_node);
            return false;
        }
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
