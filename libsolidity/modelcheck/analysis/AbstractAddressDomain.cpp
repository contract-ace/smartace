/**
 * Provides utilities for generating PTGs.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>

#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/utils/General.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

LiteralExtractor::LiteralExtractor(
    FlatModel const& _model, CallGraph const& _calls
)
{
    // Analyzes literals in state variable declarations.
    for (auto contract : _model.view())
    {
        for (auto var : contract->state_variables())
        {
            var->accept(*this);
        }
    }

    // Analyzes functions.
    for (auto func : _calls.executed_code())
    {
        ScopedSwap<CallableDeclaration const*> scope(m_context, func);
        func->body().accept(*this);
    }

    // Analyzes modifiers.
    for (auto mod : _calls.applied_modifiers())
    {
        ScopedSwap<CallableDeclaration const*> scope(m_context, mod);
        mod->body().accept(*this);
    }
}

set<dev::u256> LiteralExtractor::literals() const
{
    return m_literals;
}

list<AddressViolation> LiteralExtractor::violations() const
{
    return m_violations;
}

bool LiteralExtractor::visit(UnaryOperation const& _node)
{
    if (m_is_address_cast)
    {
        record_violation(AddressViolation::Type::Mutate, &_node);
        return false;
    }
    return true;
}

bool LiteralExtractor::visit(BinaryOperation const& _node)
{
    if (m_is_address_cast)
    {
        // TODO: Support compile-time operations.
        record_violation(AddressViolation::Type::Mutate, &_node);
        return false;
    }

    auto const& lhs = _node.leftExpression();
    if (lhs.annotation().type->category() == Type::Category::Address)
    {
        auto const TOK = _node.getOperator();
        if (TOK != Token::Equal && TOK != Token::NotEqual)
        {
            record_violation(AddressViolation::Type::Compare, &_node);
            return false;
        }
    }

    return true;
}

bool LiteralExtractor::visit(FunctionCall const& _node)
{
    auto to_type = _node.annotation().type->category();
    if (m_is_address_cast
        && (to_type != Type::Category::Contract)
        && (to_type != Type::Category::Address))
    {
        record_violation(AddressViolation::Type::Mutate, &_node);
    }
    else if (_node.annotation().kind == FunctionCallKind::TypeConversion)
    {
        auto base = _node.arguments()[0];
        auto from_type = base->annotation().type->category();
        handle_cast(*base, from_type, to_type);
    }
    else
    {
        ScopedSwap<bool> scope(m_is_address_cast, false);
        handle_call(_node);
    }
    return false;
}

bool LiteralExtractor::visit(MemberAccess const& _node)
{
    if (m_is_address_cast)
    {
        // TODO: This is "okay" if the member stores a compile-time constant.
        record_violation(AddressViolation::Type::Mutate, &_node);
        return false;
    }
    return true;
}

bool LiteralExtractor::visit(Identifier const& _node)
{
    if (m_is_address_cast && _node.name() != "this")
    {
        if (_node.annotation().type->category() != Type::Category::Contract)
        {
            // TODO: This is "okay" if the var stores a compile-time constant.
            record_violation(AddressViolation::Type::Mutate, &_node);
            return false;
        }
    }
    return true;
}

bool LiteralExtractor::visit(Literal const& _node)
{
    if (m_is_address_cast)
    {
        m_literals.insert(_node.annotation().type->literalValue(&_node));
    }
    return true;
}

void LiteralExtractor::handle_cast(
    Expression const& _base, Type::Category _from, Type::Category _to
)
{
    if (_to == Type::Category::Address)
    {
        if (_from != Type::Category::Address)
        {
            ScopedSwap<bool> scope(m_is_address_cast, true);
            _base.accept(*this);
        }
        else
        {
            _base.accept(*this);
        }
    }
    else if (_from == Type::Category::Address)
    {
        record_violation(AddressViolation::Type::Cast, &_base);
    }
    else
    {
        _base.accept(*this);
    }

}

void LiteralExtractor::handle_call(FunctionCall const& _node)
{
    // Checks for violations among the base expressions.
    _node.expression().accept(*this);


    // Checks for violations among the arguments.
    for (auto arg : _node.arguments())
    {
        arg->accept(*this);
    }
}

void LiteralExtractor::record_violation(
    AddressViolation::Type _ty, ASTNode const* _site
)
{
    m_violations.push_back(AddressViolation(_ty, m_context, _site));
}

// -------------------------------------------------------------------------- //

RoleExtractor::RoleExtractor(
    MapDeflate const& _map_db, FlatContract const& _contract
): m_map_db(_map_db)
{
    // TODO: Refine. This assumes every role is in use.
    for (auto var : _contract.state_variables())
    {
        auto type = var->type();
        if (type->category() == Type::Category::Address)
        {
            m_roles.emplace_back();
            m_roles.back().decl = var;
            m_roles.back().paths.emplace_back(list<string>{var->name()});
            m_role_ct = m_role_ct + 1;
        }
        else if (type->category() == Type::Category::Mapping)
        {
            check_map_conformance(var);
        }
        else if (type->category() == Type::Category::Struct)
        {
            auto structure = dynamic_cast<StructType const*>(var->type());

            m_roles.emplace_back();
            m_roles.back().decl = var;
            m_roles.back().paths = extract_from_struct(var->name(), structure);
        }
    }
}

void RoleExtractor::check_map_conformance(VariableDeclaration const* _decl)
{
    auto summary = m_map_db.try_resolve(*_decl);

    // Checks that keys are always addresses.
    for (auto key : summary->key_types)
    {
        auto key_type = key->annotation().type;
        if (key_type->category() != Type::Category::Address)
        {
            record_violation(AddressViolation::Type::KeyType, _decl);
            break;
        }
    }

    // Ensures that values are never addresses.
    auto value_type = summary->value_type->annotation().type;
    if (value_type->category() == Type::Category::Address)
    {
        record_violation(AddressViolation::Type::ValueType, _decl);
    }
    else if (value_type->category() == Type::Category::Struct)
    {
        auto structure = dynamic_cast<StructType const*>(value_type);

        if (!extract_from_struct("", structure).empty())
        {
            record_violation(AddressViolation::Type::ValueType, _decl);
        }
    }
}

RoleExtractor::PathSet
RoleExtractor::extract_from_struct(string _name, StructType const* _struct)
{
    list<list<string>> partial_paths;
    for (auto field : _struct->structDefinition().members())
    {
        auto category = field->type()->category();
        if (category == Type::Category::Address)
        {
            // If the field is an address, create path: <struct>.<field>
            partial_paths.push_back(list<string>{_name, field->name()});
            m_role_ct = m_role_ct + 1;
        }
        else if (category == Type::Category::Mapping)
        {
            // Maps cannot have address variables when PTGBuilder is used.
            check_map_conformance(field.get());
        }
        else if (category == Type::Category::Struct)
        {
            // For each path p in field, record: p.prepend(<struct>)
            auto structure = dynamic_cast<StructType const*>(field->type());

            auto rec_paths = extract_from_struct(field->name(), structure);
            for (auto path : rec_paths)
            {
                list<string> new_path{_name};
                new_path.splice(new_path.end(), path);
                partial_paths.push_back(std::move(new_path));
            }
        }
    }
    return partial_paths;
}

list<RoleExtractor::Role> RoleExtractor::roles() const
{
    return m_roles;
}

uint64_t RoleExtractor::count() const
{
    return m_role_ct;
}

list<AddressViolation> RoleExtractor::violations() const
{
    return m_violations;
}

void RoleExtractor::record_violation(
    AddressViolation::Type _ty, ASTNode const* _site
)
{
    m_violations.push_back(AddressViolation(_ty, nullptr, _site));
}

// -------------------------------------------------------------------------- //

ClientExtractor::ClientExtractor(FlatModel const& _model)
{
    // TODO: Refine. This assumes every client is in use.
    for (auto contract : _model.view())
    {
        // If there is a fallback method, then the minimum is 1.
        if (auto func = contract->fallback())
        {
            compute_clients(*func);
        }

        // Computes the number of client per method.
        for (auto func : contract->interface())
        {
            compute_clients(*func);
        }
    }
}

uint64_t ClientExtractor::count() const
{
    return m_client_ct;
}

void ClientExtractor::compute_clients(FunctionDefinition const& _func)
{
    // There is a minimum of one client (the sender).
    size_t potential_clients = 1;

    // Computes the maximum number of clients (over-approximate).
    for (auto param : _func.parameters())
    {
        if (param->type()->category() == Type::Category::Address)
        {
            potential_clients = potential_clients + 1;
        }
    }

    // Updates the total so far.
    if (potential_clients > m_client_ct)
    {
        m_client_ct = potential_clients;
    }
}

// -------------------------------------------------------------------------- //

PTGBuilder::PTGBuilder(
    MapDeflate const& _map_db,
    FlatModel const& _model,
    CallGraph const& _calls,
    bool _concrete,
    uint64_t _contract_ct,
    uint64_t _aux_ct
): m_concrete(_concrete), m_contract_ct(_contract_ct), m_aux_ct(_aux_ct)
{
    // Processes literals.
    {
        LiteralExtractor lext(_model, _calls);
        m_literals = lext.literals();

        for (auto violation : lext.violations())
        {
            m_violations.push_back(violation);
        }
    }

    // Processes roles.
    for (auto contract : _model.view())
    {
        RoleExtractor rext(_map_db, *contract);
        m_role_lookup[contract.get()] = rext.roles();
        m_role_ct += rext.count();

        for (auto violation : rext.violations())
        {
            m_violations.push_back(violation);
        }
    }

    // Processes clients.
    {
        ClientExtractor cext(_model);
        m_client_ct = cext.count();
    }
}

set<dev::u256> const& PTGBuilder::literals() const
{
    return m_literals;
}

list<RoleExtractor::Role>
PTGBuilder::summarize(shared_ptr<FlatContract const> _contract) const
{
    auto res = m_role_lookup.find(_contract.get());
    if (res == m_role_lookup.end())
    {
        throw runtime_error("AddressVariables queried for unknown contract");
    }
    return res->second;
}

uint64_t PTGBuilder::contract_count() const
{
    return m_contract_ct;
}

uint64_t PTGBuilder::implicit_count() const
{
    return m_contract_ct + m_literals.size() + m_aux_ct;
}

uint64_t PTGBuilder::interference_count() const
{
    return (m_concrete ? 0 : m_role_ct + m_client_ct);
}

uint64_t PTGBuilder::size() const
{
    return implicit_count() + interference_count();
}

list<AddressViolation> PTGBuilder::violations() const
{
    return m_violations;
}

// -------------------------------------------------------------------------- //

}
}
}
