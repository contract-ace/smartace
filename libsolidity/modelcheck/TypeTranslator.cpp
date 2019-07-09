/*
 * @date 2019
 * This model maps each Solidity type to a C-type. For structures and contracts,
 * these are synthesized C-structs. This translation unit provides utilities for
 * performing such conversions.
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <sstream>
#include <stdexcept>

using namespace std;
using namespace boost;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

map<string, Translation> const TypeConverter::m_global_context({
    {"abi", {}},
    {"addmod", {"unsigned int", "addmod"}},
    {"assert", {"void", "assert"}},
    {"block", {}},
    {"blockhash", {"", "blockhash"}}, // TODO(scottwe): byte32
    {"ecrecover", {"int", "ecrecover"}},
    {"gasleft", {"unsigned int", "gasleft"}},
    {"keccak256", {"", "keccak256"}}, // TODO(scottwe): byte32
    {"log0", {"void", "log0"}},
    {"log1", {"void", "log1"}},
    {"log2", {"void", "log2"}},
    {"log3", {"void", "log3"}},
    {"log4", {"void", "log4"}},
    {"msg", {}},
    {"mulmod", {"unsigned int", "mulmod"}},
    {"now", {"unsigned int", "unsigned int"}},
    {"require", {"void", "require"}},
    {"revert", {"void", "revert"}},
    {"ripemd160", {"", "ripemd160"}}, // TODO(scottwe): byte20
    {"selfdestruct", {"void", "selfdestruct"}},
    {"sha256", {"", "sha256"}}, // TODO(scottwe): byte32
    {"sha3", {"", "sha3"}}, // TODO(scottwe): byte32
    {"suicide", {"void", "suicide"}},
    {"tx", {}},
    {"type", {}}
});

// -------------------------------------------------------------------------- //

void TypeConverter::record(SourceUnit const& _unit)
{
    auto contracts = ASTNode::filteredNodes<ContractDefinition>(_unit.nodes());

    for (auto contract : contracts)
    {
        m_curr_contract = contract;

        for (auto structure : contract->definedStructs())
        {
            ostringstream struct_oss;
            struct_oss << contract->name() << "_" << structure->name();
        
            Translation struct_entry;
            struct_entry.name = struct_oss.str();
            struct_entry.type = "struct " + struct_entry.name;
            m_dictionary.insert({structure, struct_entry});
        }

        Translation contract_entry;
        contract_entry.name = contract->name();
        contract_entry.type = "struct " + contract_entry.name;
        m_dictionary.insert({contract, contract_entry});

        m_curr_contract = nullptr;
    }

    for (auto contract : contracts)
    {
        m_curr_contract = contract;

        for (auto structure : contract->definedStructs())
        {
            for (auto decl : structure->members())
            {
                decl->accept(*this);
            }
        }

        for (auto decl : contract->stateVariables())
        {
            decl->accept(*this);
        }

        for (auto fun : contract->definedFunctions())
        {
            auto const* returnParams = fun->returnParameterList().get();

            m_is_retval = true;
            returnParams->accept(*this);
            m_is_retval = false;

            ostringstream fun_oss;
            fun_oss << (fun->isConstructor() ? "Ctor" : "Method")
                    << "_" << contract->name();
            if (!fun->isConstructor())
            {
                fun_oss << "_" << fun->name();
            }

            Translation fun_entry;
            fun_entry.name = fun_oss.str();
            fun_entry.type = translate_impl(returnParams).type;
            m_dictionary.insert({fun, fun_entry});

            fun->parameterList().accept(*this);
            fun->body().accept(*this);
        }

        for (auto mod : contract->functionModifiers())
        {
            ostringstream mod_oss;
            mod_oss << "Modifier_" << contract->name() << "_" << mod->name();

            Translation mod_entry;
            mod_entry.name = mod_oss.str();
            mod_entry.type = "void";
            m_dictionary.insert({mod, mod_entry});

            mod->parameterList().accept(*this);
        }

        m_curr_contract = nullptr;
    }
}

// -------------------------------------------------------------------------- //

Translation TypeConverter::translate(ContractDefinition const& _contract) const
{
    return translate_impl(&_contract);
}

Translation TypeConverter::translate(StructDefinition const& _structure) const
{
    return translate_impl(&_structure);
}

Translation TypeConverter::translate(VariableDeclaration const& _decl) const
{
    return translate_impl(&_decl);
}

Translation TypeConverter::translate(TypeName const& _type) const
{
    return translate_impl(&_type);
}

Translation TypeConverter::translate(FunctionDefinition const& _fun) const
{
    return translate_impl(&_fun);
}

Translation TypeConverter::translate(ModifierDefinition const& _mod) const
{
    return translate_impl(&_mod);
}

Translation TypeConverter::translate(Identifier const& _id) const
{
    return translate_impl(&_id);
}

Translation TypeConverter::translate(MemberAccess const& _access) const
{
    switch (_access.expression().annotation().type->category())
    {
    case Type::Category::Struct:
    case Type::Category::Contract:
        return translate_impl(&_access);
    default:
        throw runtime_error("MemberAccess translations limited to ADT.");
    }
}

// -------------------------------------------------------------------------- //

bool TypeConverter::visit(VariableDeclaration const& _node)
{
    if (!_node.typeName())
    {
        throw runtime_error("`var` type unsupported.");
    }
    else
    {
        m_curr_decl = &_node;
        _node.typeName()->accept(*this);
        m_curr_decl = nullptr;

        if (_node.value())
        {
            _node.value()->accept(*this);
        }

        auto translation = translate_impl(_node.typeName());
        m_dictionary.insert({&_node, translation});
    }

    return false;
}

bool TypeConverter::visit(ElementaryTypeName const& _node)
{
    Translation translation;

    auto type = _node.annotation().type;
    switch (type->category())
    {
    case Type::Category::Address:
    case Type::Category::Bool:
        translation.name = "int";
        break;
    case Type::Category::Integer:
        if (dynamic_cast<IntegerType const*>(type)->isSigned())
        {
            translation.name = "int";
        }
        else
        {
            translation.name = "unsigned int";
        }
        break;
    case Type::Category::RationalNumber:
    case Type::Category::FixedPoint:
        translation.name = "double";
        break;
    default:
        throw runtime_error("Encountered ElementryTypeName with complex type.");
    }
    translation.type = translation.name;

    m_dictionary.insert({&_node, translation});

    return false;
}

bool TypeConverter::visit(UserDefinedTypeName const& _node)
{
    auto t = translate_impl(_node.annotation().referencedDeclaration);
    m_dictionary.insert({&_node, t});
    return false;
}

bool TypeConverter::visit(FunctionTypeName const& _node)
{
    (void) _node;
    throw runtime_error("Function type unsupported.");
}

bool TypeConverter::visit(Mapping const&)
{
    ++m_rectype_depth;
    return true;
}

bool TypeConverter::visit(ArrayTypeName const& _node)
{
    (void) _node;
    throw runtime_error("Array type unsupported.");
}

// -------------------------------------------------------------------------- //

void TypeConverter::endVisit(ParameterList const& _node)
{
    if (m_is_retval)
    {
        Translation translation;
        if (_node.parameters().size() > 1)
        {
            throw runtime_error("Multiple return types are unsupported.");
        }
        else if (_node.parameters().size() == 1)
        {
            translation = translate_impl(_node.parameters()[0].get());
        }
        else
        {
            translation.name = "void";
            translation.type = "void";
        }
        m_dictionary.insert({&_node, translation});
    }
}

void TypeConverter::endVisit(Mapping const& _node)
{
    string name = translate_impl(m_curr_decl->scope()).name;
    name += "_" + m_curr_decl->name() + "_submap" + to_string(m_rectype_depth);

    Translation translation;
    translation.name = name;
    translation.type = "struct " + name;
    m_dictionary.insert({&_node, translation});

    --m_rectype_depth;
}

void TypeConverter::endVisit(MemberAccess const& _node)
{
	auto expr_type = _node.expression().annotation().type;
    if (auto contract_type = dynamic_cast<ContractType const*>(expr_type))
    {
        for (auto member : contract_type->contractDefinition().stateVariables())
        {
            if (member->name() == _node.memberName())
            {
                Translation translation = translate_impl(member);
                m_dictionary.insert({&_node, translation});
                return;
            }
        }
    }
    else if (auto struct_type = dynamic_cast<StructType const*>(expr_type))
	{
        for (auto member : struct_type->structDefinition().members())
        {
            if (member->name() == _node.memberName())
            {
                Translation translation = translate_impl(member.get());
                m_dictionary.insert({&_node, translation});
                return;
            }
        }
	}
}

void TypeConverter::endVisit(Identifier const& _node)
{
    auto magic_res = m_global_context.find(_node.name());
    if (magic_res != m_global_context.end())
    {
        m_dictionary.insert({&_node, magic_res->second});
    }
    else
    {
        ASTNode const* ref = nullptr;

        if (_node.name() == "this")
        {
            ref = m_curr_contract;
        }
        else if (_node.name() == "super")
        {
            // Note: ContractDefinitionAnnotation::linearizedBaseContracts.
            // TODO(scottwe): resolve super; not needed for MVP prototype.
            throw runtime_error("super is currently unsupported.");
        }
        else
        {
            ref = _node.annotation().referencedDeclaration;
        }

        auto actual_res = translate_impl(ref);
        m_dictionary.insert({&_node, actual_res});
    }
}

// -------------------------------------------------------------------------- //

Translation TypeConverter::translate_impl(ASTNode const* _node) const
{
    auto res = m_dictionary.find(_node);
    if (res == m_dictionary.end())
    {
        throw logic_error("Translation request for type not in source unit.");
    }
    return res->second;
}

// -------------------------------------------------------------------------- //

}
}
}
