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

void TypeConverter::record(SourceUnit const& _unit)
{
    auto contracts = ASTNode::filteredNodes<ContractDefinition>(_unit.nodes());

    for (auto contract : contracts)
    {
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
    }

    for (auto contract : contracts)
    {
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

// -------------------------------------------------------------------------- //

bool TypeConverter::visit(VariableDeclaration const& _node)
{
    m_curr_decl = &_node;
    if (!_node.typeName())
    {
        throw runtime_error("`var` type unsupported.");
    }
    else
    {
        _node.typeName()->accept(*this);

        auto translation = translate_impl(_node.typeName());
        m_dictionary.insert({&_node, translation});
    }
    m_curr_decl = nullptr;

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
