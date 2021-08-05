/**
 * Provides utilities for generating PTGs.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>

#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/Mapping.h>
#include <libsolidity/modelcheck/analysis/TaintAnalysis.h>
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

vector<AddressViolation> LiteralExtractor::violations() const
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
    CallGraph const& _calls,
    MapDeflate const& _map_db,
    FlatContract const& _contract
): m_map_db(_map_db)
{
    // Collects the roles.
    std::vector<VariableDeclaration const*> pub_role_vars;
    std::vector<VariableDeclaration const*> pri_role_vars;
    for (auto var : _contract.state_variables())
    {
        auto type = var->type();
        auto & role_vars = (var->isPublic() ? pub_role_vars : pri_role_vars);
        if (type->category() == Type::Category::Address)
        {
            m_roles.emplace_back();
            m_roles.back().decl = var;
            m_roles.back().paths.emplace_back(Path{var->name()});
            role_vars.push_back(var);
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
            m_roles.back().paths
                = extract_from_struct(var->name(), role_vars, structure);
        }
    }

    // Describes taint analysis.
    vector<bool> tainted(pri_role_vars.size(), false);
    auto analyze = [&tainted,&pri_role_vars]
                   (set<FunctionDefinition const*> _funcs)
    {
        for (auto func : _funcs)
        {
            RoleTaintPass analysis(*func, pri_role_vars);
            auto const& result = analysis.extract();
            for (size_t i = 0; i < result.size(); ++i)
            {
                if (result[i])
                {
                    tainted[i] = true;
                }
            }
        }
    };

    // Performs taint analysis.
    analyze(_calls.internals(_contract));
    for (auto func : _contract.interface())
    {
        analyze(_calls.super_calls(_contract, *func));
    }

    // Computes number of roles.
    m_role_ct = pub_role_vars.size();
    for (auto v : tainted)
    {
        if (v)
        {
            m_role_ct += 1;
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
        vector<VariableDeclaration const*> tmp;

        if (!extract_from_struct("", tmp, structure).empty())
        {
            record_violation(AddressViolation::Type::ValueType, _decl);
        }
    }
}

RoleExtractor::PathGroup RoleExtractor::extract_from_struct(
    string _name,
    vector<VariableDeclaration const*> & _role_vars,
    StructType const* _struct
)
{
    PathGroup partial_paths;
    for (auto field : _struct->structDefinition().members())
    {
        auto category = field->type()->category();
        if (category == Type::Category::Address)
        {
            // If the field is an address, create path: <struct>.<field>
            partial_paths.push_back(Path{_name, field->name()});
            _role_vars.push_back(field.get());
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

            auto rec_paths
                = extract_from_struct(field->name(), _role_vars, structure);
            for (auto path : rec_paths)
            {
                Path new_path{_name};
                new_path.splice(new_path.end(), path);
                partial_paths.push_back(std::move(new_path));
            }
        }
    }
    return partial_paths;
}

vector<RoleExtractor::Role> RoleExtractor::roles() const
{
    return m_roles;
}

uint64_t RoleExtractor::count() const
{
    return m_role_ct;
}

vector<AddressViolation> RoleExtractor::violations() const
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
    // Without a sender, behaviours are lost, so there is at least one client.
    size_t potential_clients = 1;

    // Taint analysis to over-approximate the number of clients.
    ClientTaintPass analysis(_func);
    auto const& taint = analysis.extract();
    for (auto v : taint)
    {
        if (v)
        {
            potential_clients += 1;
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
    uint64_t _inf_ct,
    uint64_t _aux_ct
): m_concrete(_concrete)
 , m_contract_ct(_contract_ct)
 , m_inf_ct(_inf_ct)
 , m_aux_ct(_aux_ct)
{
    // Processes literals.
    {
        LiteralExtractor lext(_model, _calls);
        m_literals = lext.literals();
        add_violations(lext.violations());
    }

    // Processes roles.
    for (auto contract : _model.view())
    {
        RoleExtractor rext(_calls, _map_db, *contract);
        m_role_lkup[contract.get()] = rext.roles();
        m_role_ct += rext.count();
        add_violations(rext.violations());
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

vector<RoleExtractor::Role>
PTGBuilder::summarize(shared_ptr<FlatContract const> _contract) const
{
    auto res = m_role_lkup.find(_contract.get());
    if (res == m_role_lkup.end())
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

uint64_t PTGBuilder::max_sender() const
{
    return implicit_count() + interference_count();
}

uint64_t PTGBuilder::count() const
{
    return max_sender() + m_inf_ct;
}

vector<AddressViolation> PTGBuilder::violations() const
{
    return m_violations;
}

void PTGBuilder::add_violations(vector<AddressViolation> const& _violations)
{
    m_violations.reserve(m_violations.size() + _violations.size());
    for (auto violation : _violations)
    {
        m_violations.push_back(violation);
    }
}

// -------------------------------------------------------------------------- //

}
}
}
