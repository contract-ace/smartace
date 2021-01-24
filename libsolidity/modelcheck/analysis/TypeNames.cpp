#include <libsolidity/modelcheck/analysis/TypeNames.h>

#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/AST.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/Types.h>

#include <sstream>
#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

map<string, string> const TypeAnalyzer::m_global_context_types({
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

set<string> const TypeAnalyzer::m_global_simple_values({"now"});

// -------------------------------------------------------------------------- //

void TypeAnalyzer::record(SourceUnit const& _unit)
{
    auto contracts = ASTNode::filteredNodes<ContractDefinition>(_unit.nodes());

    // Pass 1: assign types to all contracts and structures.
    for (auto contract : contracts)
    {
        ScopedSwap<ContractDefinition const*> swap(m_curr_contract, contract);
        string cname = escape_decl_name(*contract);

        for (auto e : contract->definedEnums())
        {
            m_type_lookup.insert({e, get_simple_ctype(*e->type())});
        }

        for (auto structure : contract->definedStructs())
        {
            ostringstream struct_oss;
            struct_oss << cname << "_Struct_" << escape_decl_name(*structure);

            m_name_lookup.insert({structure, struct_oss.str()});
            m_type_lookup.insert({structure, "struct " + struct_oss.str()});
        }

        m_name_lookup.insert({contract, cname});
        m_type_lookup.insert({contract, "struct " + cname});
    }

    // Pass 2: assign types to all member fields and methods, such that their
    // types may be referenced within function bodies.
    for (auto con : contracts)
    {
        ScopedSwap<ContractDefinition const*> swap(m_curr_contract, con);

        for (auto structure : con->definedStructs())
        {
            for (auto decl : structure->members())
            {
                decl->accept(*this);
            }
        }

        for (auto decl : con->stateVariables())
        {
            decl->accept(*this);
        }

        if (!con->isInterface())
        {
            for (auto fun : con->definedFunctions())
            {
                fun->parameterList().accept(*this);
                if (fun->isConstructor()) continue;

                auto const* returnParams = fun->returnParameterList().get();
                {
                    ScopedSwap<bool> swap(m_is_retval, true);
                    returnParams->accept(*this);
                }

                // TODO: is this still used?
                auto const FUNC_RETURN_TYPE = get_type(*returnParams);
                auto const FUNC_NAME = FunctionSpecialization(*fun).name(0);
                m_name_lookup.insert({fun, FUNC_NAME});
                m_type_lookup.insert({fun, FUNC_RETURN_TYPE});
            }

            for (auto modifier : con->functionModifiers())
            {
                modifier->parameterList().accept(*this);
            }
        }
    }

    // Pass 3: assign types to Solidity expressions, where applicable.
    for (auto contract : contracts)
    {
        if (contract->isInterface()) continue;

        ScopedSwap<ContractDefinition const*> swap(m_curr_contract, contract);

        for (auto fun : contract->definedFunctions())
        {
            if (!fun->isImplemented()) continue;
            fun->body().accept(*this);
        }

        for (auto modifier : contract->functionModifiers())
        {
            modifier->body().accept(*this);
        }
    }
}

// -------------------------------------------------------------------------- //

bool TypeAnalyzer::is_pointer(Identifier const& _id) const
{
    auto const& RES = m_in_storage.find(&_id);
    return RES != m_in_storage.end() && RES->second;
}

string TypeAnalyzer::get_type(ASTNode const& _node) const
{
    if (m_type_lookup.find(&_node) == m_type_lookup.end())
    {
        string name = get_error_type(&_node);
        throw runtime_error("get_type called on unknown ASTNode: " + name);
    }
    return m_type_lookup.find(&_node)->second;
}

string TypeAnalyzer::get_name(ASTNode const& _node) const
{
    auto const& RES = m_name_lookup.find(&_node);
    if (RES == m_name_lookup.end())
    {
        string name = get_error_type(&_node);
        throw runtime_error("get_name called on unknown ASTNode: " + name);
    }
    return RES->second;
}

// -------------------------------------------------------------------------- //

string TypeAnalyzer::get_simple_ctype(Type const& _type)
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
    else if (auto fixed_ptr = dynamic_cast<FixedPointType const*>(&type))
    {
        ostringstream numeric_oss;
        numeric_oss << "sol_";
        if (!fixed_ptr->isSigned()) numeric_oss << "u";
        numeric_oss << "fixed" << fixed_ptr->numBits()
                    << "X" << fixed_ptr->fractionalDigits() << "_t";
        return numeric_oss.str();
    }
    else if (auto byte_ptr = dynamic_cast<FixedBytesType const*>(&type))
    {
        uint16_t bits = byte_ptr->numBytes() * 8;
        ostringstream numeric_oss;
        numeric_oss << "sol_uint" << bits << "_t";
        return numeric_oss.str();
    }
    else if (auto array_ptr = dynamic_cast<ArrayType const*>(&type))
    {
        if (array_ptr->isString()) return "sol_uint256_t";
    }

    string const TYPE_NAME = type.canonicalName();
    throw runtime_error("Unable to resolve simple type from: " + TYPE_NAME);
}

// -------------------------------------------------------------------------- //

CExprPtr TypeAnalyzer::init_val_by_simple_type(Type const& _type)
{
    if (!is_simple_type(_type))
    {
        throw ("init_val_by_simple_type expects a simple type.");
    }
    return InitFunction::wrap(_type, Literals::ZERO);
}

CExprPtr TypeAnalyzer::get_init_val(TypeName const& _typename) const
{
    if (has_simple_type(_typename))
    {
        return init_val_by_simple_type(*_typename.annotation().type);
    }
    return InitFunction(*this, _typename).defaulted();
}

CExprPtr TypeAnalyzer::get_init_val(Declaration const& _decl) const
{
    if (has_simple_type(_decl)) return init_val_by_simple_type(*_decl.type());
    return InitFunction(*this, _decl).defaulted();
}

// -------------------------------------------------------------------------- //

MapDeflate TypeAnalyzer::map_db() const { return m_map_db; }

// -------------------------------------------------------------------------- //

bool TypeAnalyzer::visit(VariableDeclaration const& _node)
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

bool TypeAnalyzer::visit(ElementaryTypeName const& _node)
{
    m_type_lookup.insert({&_node, get_simple_ctype(*_node.annotation().type)});
    return false;
}

bool TypeAnalyzer::visit(UserDefinedTypeName const& _node)
{
    auto const& REF = *_node.annotation().referencedDeclaration;
    m_type_lookup.insert({&_node, get_type(REF)});
    if (!has_simple_type(REF))
    {
        m_name_lookup.insert({&_node, get_name(REF)});
    }
    return false;
}

bool TypeAnalyzer::visit(FunctionTypeName const& _node)
{
    (void) _node;
    throw runtime_error("Function type unsupported.");
}

bool TypeAnalyzer::visit(Mapping const& _node)
{
    auto const& record = m_map_db.query(_node);
    m_name_lookup.insert({&_node, record->name});
    m_type_lookup.insert({&_node, "struct " + record->name});

    for (auto const* key : record->key_types) key->accept(*this);
    record->value_type->accept(*this);

    return false;
}

bool TypeAnalyzer::visit(ArrayTypeName const& _node)
{
    (void) _node;
    throw runtime_error("Array type unsupported.");
}

bool TypeAnalyzer::visit(IndexAccess const& _node)
{
    FlatIndex idx(_node);
    auto const& record = m_map_db.resolve(idx.decl());

    m_type_lookup.insert({&_node, get_type(*record->value_type)});
    m_name_lookup.insert({&_node, record->name});

    for (auto const* idx_expr : idx.indices()) idx_expr->accept(*this);
    idx.base().accept(*this);

    return false;
}

bool TypeAnalyzer::visit(EmitStatement const&) { return false; }

bool TypeAnalyzer::visit(EventDefinition const&) { return false; }

// -------------------------------------------------------------------------- //

void TypeAnalyzer::endVisit(ParameterList const& _node)
{
    if (m_is_retval)
    {
        string ctype;
        if (_node.parameters().size() > 0)
        {
            // We treat the first return value as the canonical return type.
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

void TypeAnalyzer::endVisit(MemberAccess const& _node)
{
    if (auto decl = member_access_to_decl(_node))
    {
        m_type_lookup.insert({&_node, get_type(*decl)});
        if (!has_simple_type(*decl))
        {
            m_name_lookup.insert({&_node, get_name(*decl)});
        }
    }
}

void TypeAnalyzer::endVisit(Identifier const& _node)
{
    auto const& NODE_NAME = _node.name();
    if (NODE_NAME == "super") return;

    auto const MAGIC_RES = m_global_context_types.find(NODE_NAME);
    if (MAGIC_RES != m_global_context_types.end())
    {
        m_type_lookup.insert({&_node, MAGIC_RES->second});
        m_in_storage.insert({&_node, false});

        auto const MAGIC_SIMPLE = m_global_simple_values.find(NODE_NAME);
        if (MAGIC_SIMPLE != m_global_simple_values.end())
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
