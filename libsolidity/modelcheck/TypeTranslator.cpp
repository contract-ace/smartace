/*
 * @date 2019
 * This model maps each Solidity type to a C-type. For structures and contracts,
 * these are synthesized C-structs. This translation unit provides utilities for
 * performing such conversions.
 */

#include <libsolidity/modelcheck/TypeTranslator.h>

#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/TypeClassification.h>
#include <libsolidity/modelcheck/Utility.h>
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

map<string, string> const TypeConverter::m_global_context_types({
    {"abi", ""}, {"addmod", "unsigned int"}, {"assert", "void"}, {"block", ""},
    {"blockhash", ""/*TODO(scottwe): byte32*/}, {"ecrecover", "int"},
    {"gasleft", "unsigned int"}, {"keccak256", ""/*TODO(scottwe): byte32*/},
    {"log0", "void"}, {"log1", "void"}, {"log2", "void"}, {"log3", "void"},
    {"log4", "void"}, {"msg", ""}, {"mulmod", "unsigned int"},
    {"now", "unsigned int"}, {"require", "require"}, {"revert", "void"},
    {"ripemd160", ""/*TODO(scottwe): byte20*/}, {"selfdestruct", "void"},
    {"sha256", ""/*TODO(scottwe): byte32*/}, {"type", ""}, {"tx", ""},
    {"sha3", ""/*TODO(scottwe): byte32*/}, {"suicide", "void"}
});

set<string> const TypeConverter::m_global_context_simple_values({"now"});

// -------------------------------------------------------------------------- //

AccessDepthResolver::AccessDepthResolver(IndexAccess const& _base)
: m_base(_base) {}

Mapping const* AccessDepthResolver::resolve()
{
    m_decl = nullptr;
    m_submap_count = 0;
    m_base.accept(*this);

    Mapping const* mapping = nullptr;
    if (m_decl)
    {
        mapping = dynamic_cast<Mapping const*>(m_decl->typeName());
        for (unsigned int i = 1; (i < m_submap_count) && mapping; ++i)
        {
            mapping = dynamic_cast<Mapping const*>(&mapping->valueType());
        }
    }
    return mapping;
}

bool AccessDepthResolver::visit(Conditional const&)
{
	throw std::runtime_error("Conditional map accesses are unsupported.");
}

bool AccessDepthResolver::visit(MemberAccess const& _node)
{
    auto const EXPR_TYPE = _node.expression().annotation().type;
    if (auto contract_type = dynamic_cast<ContractType const*>(EXPR_TYPE))
    {
        for (auto member : contract_type->contractDefinition().stateVariables())
        {
            if (member->name() == _node.memberName())
            {
                m_decl = member;
                break;
            }
        }
    }
    else if (auto struct_type = dynamic_cast<StructType const*>(EXPR_TYPE))
    {
        for (auto member : struct_type->structDefinition().members())
        {
            if (member->name() == _node.memberName())
            {
                m_decl = member.get();
                break;
            }
        }
    }
    return false;
}

bool AccessDepthResolver::visit(IndexAccess const& _node)
{
	++m_submap_count;
	_node.baseExpression().accept(*this);
	return false;
}

bool AccessDepthResolver::visit(Identifier const& _node)
{
    m_decl = dynamic_cast<VariableDeclaration const*>(
        _node.annotation().referencedDeclaration
    );
	return false;
}

// -------------------------------------------------------------------------- //

void TypeConverter::record(SourceUnit const& _unit)
{
    auto contracts = ASTNode::filteredNodes<ContractDefinition>(_unit.nodes());

    // Pass 1: assign types to all contracts and structures.
    for (auto contract : contracts)
    {
        ScopedSwap<ContractDefinition const*> swap(m_curr_contract, contract);
        string cname = escape_decl_name(*contract);

        for (auto structure : contract->definedStructs())
        {
            ostringstream struct_oss;
            struct_oss << cname << "_Struct" << escape_decl_name(*structure);

            m_name_lookup.insert({structure, struct_oss.str()});
            m_type_lookup.insert({structure, "struct " + struct_oss.str()});
        }

        m_name_lookup.insert({contract, cname});
        m_type_lookup.insert({contract, "struct " + cname});
    }

    // Pass 2: assign types to all member fields and methods, such that their
    // types may be referenced within function bodies.
    for (auto contract : contracts)
    {
        ScopedSwap<ContractDefinition const*> swap(m_curr_contract, contract);

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

            {
                ScopedSwap<bool> swap(m_is_retval, true);
                returnParams->accept(*this);
            }

            ostringstream fun_oss;
            fun_oss << (fun->isConstructor() ? "Ctor" : "Method") << "_"
                    << escape_decl_name(*contract);

            if (!fun->isConstructor())
            {
                fun_oss << "_Func" << escape_decl_name(*fun);
            }

            auto const FUNC_RETURN_TYPE = get_type(*returnParams);
            auto const FUNC_NAME = fun_oss.str();
            m_name_lookup.insert({fun, FUNC_NAME});
            m_type_lookup.insert({fun, FUNC_RETURN_TYPE});

            for (unsigned int i = 0; i < fun->modifiers().size(); ++i)
            {
                ostringstream mod_oss;
                mod_oss << FUNC_NAME << "_mod" << i;

                ModifierInvocation const* modifier = fun->modifiers()[i].get();
                m_name_lookup.insert({modifier, mod_oss.str()});
                m_type_lookup.insert({modifier, FUNC_RETURN_TYPE});
            }

            fun->parameterList().accept(*this);
        }
    }

    // Pass 3: assign types to Solidity expressions, where applicable.
    for (auto contract : contracts)
    {
        ScopedSwap<ContractDefinition const*> swap(m_curr_contract, contract);

        for (auto fun : contract->definedFunctions())
        {
            fun->body().accept(*this);
        }
    }
}

// -------------------------------------------------------------------------- //

bool TypeConverter::is_pointer(Identifier const& _id) const
{
    auto const& RES = m_in_storage.find(&_id);
    return RES != m_in_storage.end() && RES->second;
}

bool TypeConverter::has_record(ASTNode const& _node) const
{
    return m_type_lookup.find(&_node) != m_type_lookup.end();
}

string TypeConverter::get_type(ASTNode const& _node) const
{
    if (!has_record(_node))
    {
        string name = "unknown";
        if (auto DECL = dynamic_cast<Declaration const*>(&_node))
        {
            name = DECL->name();
        }
        throw runtime_error("get_type called on unknown ASTNode: " + name);
    }
    return m_type_lookup.find(&_node)->second;
}

string TypeConverter::get_name(ASTNode const& _node) const
{
    auto const& RES = m_name_lookup.find(&_node);
    if (RES == m_name_lookup.end())
    {
        string name = "unknown";
        if (auto DECL = dynamic_cast<Declaration const*>(&_node))
        {
            name = DECL->name();
        }
        throw runtime_error("get_name called on unknown ASTNode: " + name);
    }
    return RES->second;
}

// -------------------------------------------------------------------------- //

string TypeConverter::get_simple_ctype(Type const& _type)
{
    Type const& type = unwrap(_type);

    if (type.category() == Type::Category::Address) return "sol_address_t";
    if (type.category() == Type::Category::Bool) return "sol_bool_t";

    if (auto int_ptr = dynamic_cast<IntegerType const*>(&type))
    {
        ostringstream numeric_oss;
        numeric_oss << "sol_";
        if (!int_ptr->isSigned()) numeric_oss << "u";
        numeric_oss << "int" << int_ptr->numBits() << "_t";
        return numeric_oss.str();
    }
    if (auto fixed_ptr = dynamic_cast<FixedPointType const*>(&type))
    {
        ostringstream numeric_oss;
        numeric_oss << "sol_";
        if (!fixed_ptr->isSigned()) numeric_oss << "u";
        numeric_oss << "fixed" << fixed_ptr->numBits()
                    << "X" << fixed_ptr->fractionalDigits() << "_t";
        return numeric_oss.str();
    }

    throw runtime_error("Unable to resolve simple type from _type.");
}

// -------------------------------------------------------------------------- //

CExprPtr TypeConverter::init_val_by_simple_type(Type const& _type)
{
    if (!is_simple_type(_type))
    {
        throw ("init_val_by_simple_type expects a simple type.");
    }

    CExprPtr init_val = make_shared<CIntLiteral>(0);
    if (is_wrapped_type(_type))
    {
        string const INIT_CALL = "Init_" + get_simple_ctype(_type);
        init_val = make_shared<CFuncCall>(INIT_CALL, CArgList{init_val});
    }
    return init_val;
}

CExprPtr TypeConverter::nd_val_by_simple_type(
    Type const& _type, string const& _msg
)
{
    if (!is_simple_type(_type))
    {
        throw ("nd_val_by_simple_type expects a simple type.");
    }

    ostringstream call;
    call << "nd_";
    if (!simple_is_signed(_type)) call << "u";
    call << "int" << simple_bit_count(_type) << "_t";

    auto msg_lit = make_shared<CStringLiteral>(_msg);
    auto nd_val = make_shared<CFuncCall>(call.str(), CArgList{msg_lit});
    if (is_wrapped_type(_type))
    {
        string const INIT_CALL = "Init_" + get_simple_ctype(_type);
        nd_val = make_shared<CFuncCall>(INIT_CALL, CArgList{nd_val});
    }
    return nd_val;
}

CExprPtr TypeConverter::get_init_val(TypeName const& _typename) const
{
    if (has_simple_type(_typename))
    {
        return TypeConverter::init_val_by_simple_type(
            *_typename.annotation().type
        );
    }
    return make_shared<CFuncCall>("Init_0_" + get_name(_typename), CArgList{});
}

CExprPtr TypeConverter::get_init_val(Declaration const& _decl) const
{
    if (has_simple_type(_decl)) return init_val_by_simple_type(*_decl.type());
    return make_shared<CFuncCall>("Init_0_" + get_name(_decl), CArgList{});
}

CExprPtr TypeConverter::get_nd_val(
    TypeName const& _typename, string const& _msg
) const
{
    if (has_simple_type(_typename))
    {
        return TypeConverter::nd_val_by_simple_type(
            *_typename.annotation().type, _msg
        );
    }
    return make_shared<CFuncCall>("ND_" + get_name(_typename), CArgList{});
}

CExprPtr TypeConverter::get_nd_val(
    Declaration const& _decl, string const& _msg
) const
{
    if (has_simple_type(_decl))
    {
        return nd_val_by_simple_type(*_decl.type(), _msg);
    }
    return make_shared<CFuncCall>("ND_" + get_name(_decl), CArgList{});
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
        if (_node.value())
        {
            _node.value()->accept(*this);
        }

        ScopedSwap<VariableDeclaration const*> decl_swap(m_curr_decl, &_node);
        auto const& VAR_TYPENAME = *_node.typeName();
        VAR_TYPENAME.accept(*this);
        m_type_lookup.insert({&_node, get_type(VAR_TYPENAME)});
        if (!has_simple_type(VAR_TYPENAME))
        {
            m_name_lookup.insert({&_node, get_name(VAR_TYPENAME)});
        }
    }

    return false;
}

bool TypeConverter::visit(ElementaryTypeName const& _node)
{
    m_type_lookup.insert({&_node, get_simple_ctype(*_node.annotation().type)});
    return false;
}

bool TypeConverter::visit(UserDefinedTypeName const& _node)
{
    auto const& REF = *_node.annotation().referencedDeclaration;
    m_type_lookup.insert({&_node, get_type(REF)});
    if (!has_simple_type(REF))
    {
        m_name_lookup.insert({&_node, get_name(REF)});
    }
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
        string ctype;
        if (_node.parameters().size() > 1)
        {
            throw runtime_error("Multiple return types are unsupported.");
        }
        else if (_node.parameters().size() == 1)
        {
            auto const& PARAM = *_node.parameters()[0];
            ctype = get_type(PARAM);
            if (!has_simple_type(PARAM))
            {
                m_name_lookup.insert({&_node, get_name(PARAM)});
            }
        }
        else
        {
            ctype = "void";
        }
        m_type_lookup.insert({&_node, ctype});
    }
}

void TypeConverter::endVisit(Mapping const& _node)
{
    ostringstream name_oss;
    name_oss << get_name(*m_curr_decl->scope())
             << "_Map" + escape_decl_name(*m_curr_decl)
             << "_submap" << to_string(m_rectype_depth);

    m_name_lookup.insert({&_node, name_oss.str()});
    m_type_lookup.insert({&_node, "struct " + name_oss.str()});

    --m_rectype_depth;
}

void TypeConverter::endVisit(MemberAccess const& _node)
{
	auto const EXPR_TYPE = _node.expression().annotation().type;
    if (auto contract_type = dynamic_cast<ContractType const*>(EXPR_TYPE))
    {
        for (auto member : contract_type->contractDefinition().stateVariables())
        {
            if (member->name() == _node.memberName())
            {
                m_type_lookup.insert({&_node, get_type(*member)});
                if (!has_simple_type(*member))
                {
                    m_name_lookup.insert({&_node, get_name(*member)});
                }
                return;
            }
        }
    }
    else if (auto struct_type = dynamic_cast<StructType const*>(EXPR_TYPE))
	{
        for (auto member : struct_type->structDefinition().members())
        {
            if (member->name() == _node.memberName())
            {
                m_type_lookup.insert({&_node, get_type(*member)});
                if (!has_simple_type(*member))
                {
                    m_name_lookup.insert({&_node, get_name(*member)});
                }
                return;
            }
        }
	}
}

void TypeConverter::endVisit(IndexAccess const& _node)
{
    auto mapping = AccessDepthResolver(_node).resolve();
    if (!mapping)
    {
        throw runtime_error("Cannot resolve Mapping from IndexAccess.");
    }

    m_type_lookup.insert({&_node, get_type(mapping->valueType())});
    m_name_lookup.insert({&_node, get_name(*mapping)});
}

void TypeConverter::endVisit(Identifier const& _node)
{
    auto const& NODE_NAME = _node.name();
    auto const MAGIC_RES = m_global_context_types.find(NODE_NAME);
    if (MAGIC_RES != m_global_context_types.end())
    {
        m_type_lookup.insert({&_node, MAGIC_RES->second});
        m_in_storage.insert({&_node, false});

        auto const MAGIC_SIMP = m_global_context_simple_values.find(NODE_NAME);
        if (MAGIC_SIMP != m_global_context_simple_values.end())
        {
            m_name_lookup.insert({&_node, NODE_NAME});
        }
    }
    else
    {
        Declaration const* ref = nullptr;
        auto loc = VariableDeclaration::Location::Unspecified;

        if (NODE_NAME == "this")
        {
            ref = m_curr_contract;
            loc = VariableDeclaration::Storage;
        }
        else if (NODE_NAME == "super")
        {
            // Note: ContractDefinitionAnnotation::linearizedBaseContracts.
            // TODO(scottwe): resolve super; not needed for MVP prototype.
            throw runtime_error("super is currently unsupported.");
        }
        else
        {
            ref = _node.annotation().referencedDeclaration;
            if (auto var = dynamic_cast<VariableDeclaration const*>(ref))
            {
                loc = var->referenceLocation();
            }
        }

        m_type_lookup.insert({&_node, get_type(*ref)});
        m_in_storage.insert({&_node, loc == VariableDeclaration::Storage});
        if (!has_simple_type(*ref))
        {
            m_name_lookup.insert({&_node, get_name(*ref)});
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
