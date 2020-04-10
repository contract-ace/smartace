/**
 * @date 2020
 * Provides utilities for generating an abstract address space for map indices.
 */

#include <libsolidity/modelcheck/analysis/MapIndex.h>

#include <libsolidity/modelcheck/utils/General.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

list<list<string>> AddressVariables::analyze_struct(
    string _name, StructType const* _struct
)
{
    list<StructDefinition const*> frontier;
    frontier.push_back(&_struct->structDefinition());

    list<list<string>> partial_paths;
    partial_paths.push_back(list<string>{_name});
    
    list<list<string>> paths;
    while (!frontier.empty())
    {
        auto structure = frontier.back();
        frontier.pop_back();

        auto partial = partial_paths.back();
        partial_paths.pop_back();

        for (auto subvar : structure->members())
        {
            auto subvarcat = subvar->type()->category();
            if (subvarcat == Type::Category::Address)
            {
                auto path = partial;
                path.push_back(subvar->name());
                paths.emplace_back(move(path));
            }
            else if (subvarcat == Type::Category::Struct)
            {
                _struct = unroll<StructType>(subvar->type());

                frontier.push_back(&_struct->structDefinition());
                partial_paths.push_back(partial);
            }
        }
    }

    return paths;
}

AddressVariables::AddressEntry AddressVariables::analyze_map(
    VariableDeclaration const& _decl
)
{
    size_t depth = 1;
    bool uses_address_keys = true;

    // Unrolls mapping.
    auto mapping = unroll<MappingType>(_decl.type());
    while (true)
    {
        // Checks key type.
        if (mapping->keyType()->category() != Type::Category::Address)
        {
            uses_address_keys = false;
        }

        // Checks value type.
        if (mapping->valueType()->category() == Type::Category::Mapping)
        {
            mapping = unroll<MappingType>(mapping->valueType());
            ++depth;
        }
        else
        {
            break;
        }
    }

    // Analyzes return type.
    AddressEntry map;
    map.decl = (&_decl);
    map.depth = depth;
    map.address_only = uses_address_keys;
    if (mapping->valueType()->category() == Type::Category::Address)
    {
        map.paths.emplace_back(list<string>{_decl.name()});
    }
    else if (mapping->valueType()->category() == Type::Category::Struct)
    {
        auto structure = unroll<StructType>(mapping->valueType());
        map.paths = analyze_struct(_decl.name(), structure);
    }
    return map;
}

void AddressVariables::record(ContractDefinition const& _src)
{
    // Skips interfaces which do not add functions or variables.
    // Note that they are not skipped recursively.
    if (_src.isInterface()) return;

    m_cache[&_src] = {};

    set<string> symbols;
    for (auto CONTRACT : _src.annotation().linearizedBaseContracts)
    {
        for (auto VAR : CONTRACT->stateVariables())
        {
            // Ensures this is the first instance of the variable.
            if (symbols.find(VAR->name()) != symbols.end()) continue;
            symbols.insert(VAR->name());

            // Classifies variable.
            if (VAR->type()->category() == Type::Category::Address)
            {
                AddressEntry entry;
                entry.address_only = true;
                entry.decl = VAR;
                entry.depth = 0;
                entry.paths.emplace_back(list<string>{VAR->name()});
                m_cache[&_src].emplace_back(move(entry));
            }
            else if (VAR->type()->category() == Type::Category::Mapping)
            {
                m_cache[&_src].push_back(analyze_map(*VAR));
            }
            else if (VAR->type()->category() == Type::Category::Struct)
            {
                auto structure = unroll<StructType>(VAR->type());

                AddressEntry entry;
                entry.address_only = true;
                entry.decl = VAR;
                entry.depth = 0;
                entry.paths = analyze_struct(VAR->name(), structure);
                m_cache[&_src].emplace_back(move(entry));
            }
        }
    }
}

list<AddressVariables::AddressEntry> const& AddressVariables::access(
    ContractDefinition const& _src
) const
{
    auto res = m_cache.find(&_src);
    if (res == m_cache.end())
    {
        throw runtime_error("AddressVariables queried for unknown contract");
    }
    return res->second;
}

// -------------------------------------------------------------------------- //

MapIndexSummary::MapIndexSummary(
    bool _concrete, uint64_t _clients, uint64_t _contracts
)
    : IS_CONCRETE(_concrete)
    , m_client_reps(_clients)
    , m_contract_reps(_contracts)
    , m_max_inteference(0)
    , m_in_first_pass(false)
    , m_is_address_cast(false)
    , m_uses_contract_address(false)
    , m_context(nullptr)
{
}

void MapIndexSummary::extract_literals(ContractDefinition const& _src)
{
    // Skips interfaces which do not add functions or variables.
    if (_src.isInterface()) return;

    // Determines all addresses in contract.
    m_cache.record(_src);

    // Processes addresses relevant to literals.
    for (auto entry : m_cache.access(_src))
    {
        if (!entry.address_only)
        {
            m_violations.emplace_front();
            m_violations.front().context = nullptr;
            m_violations.front().site = entry.decl;
            m_violations.front().type = ViolationType::KeyType;
        }

        if (entry.decl->type()->category() == Type::Category::Address)
        {
            entry.decl->accept(*this);
        }
        else if (entry.paths.size() > 0)
        {
            m_literals.insert(0);
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
    // Skips interfaces which do not add functions or variables.
    // Note that they are not skipped recursively.
    // TODO: perhaps this should be factored out for reuse.
    if (_src.isInterface()) return;

    // Summarizes state address variables.
    uint64_t address_var_count = 0;
    for (auto entry : m_cache.access(_src))
    {
        if (entry.depth > 0)
        {
            if (entry.address_only)
            {
                uint64_t combs = fast_pow(representative_count(), entry.depth);
                address_var_count += (combs * entry.paths.size());
            }
        }
        else
        {
            address_var_count += entry.paths.size();
        }
    }

    // Summarizes functions.
    for (auto CONTRACT : _src.annotation().linearizedBaseContracts)
    {
        if (CONTRACT->isInterface()) break;

        for (auto const* FUNC : CONTRACT->definedFunctions())
        {
            if (!FUNC->isImplemented()) continue;

            // All state addresses, along with the sender may be interference.
            if (FUNC->isPublic())
            {
                uint64_t potential_interference = address_var_count + 1;
                for (auto var : FUNC->parameters())
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
}

list<AddressVariables::AddressEntry> const& MapIndexSummary::describe(
    ContractDefinition const& _src
) const
{
    return m_cache.access(_src);
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

uint64_t MapIndexSummary::contract_count() const
{
    return m_contract_reps;
}

uint64_t MapIndexSummary::max_interference() const
{
    if (IS_CONCRETE)
    {
        return 0;
    }
    else
    {
        return m_max_inteference;
    }
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
