/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/BlockConverter.h>

#include <libsolidity/modelcheck/ExpressionConverter.h>
#include <libsolidity/modelcheck/FunctionConverter.h>
#include <libsolidity/modelcheck/Utility.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

FunctionConverter::FunctionConverter(
    ASTNode const& _ast,
	TypeConverter const& _converter,
    bool _forward_declare
): m_ast(&_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void FunctionConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast->accept(*this);
}

// -------------------------------------------------------------------------- //

bool FunctionConverter::visit(ContractDefinition const& _node)
{
    auto translation = m_converter.translate(_node);

    (*m_ostream) << translation.type << " Init_" << translation.name;
    if (auto ctor = _node.constructor())
    {
        print_args(ctor->parameters(), &_node, false);
    }
    else
    {
        print_args({}, nullptr, false);
    }

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << translation.type << " tmp;" << endl;

        for (auto decl : _node.stateVariables())
        {
            (*m_ostream) << "tmp.d_" << decl->name() << " = ";
            if (decl->value())
            {
                ExpressionConverter(*decl->value(), {}, {}).print(*m_ostream);
            }
            else
            {
                auto decl_name = m_converter.translate(*decl).name;
                (*m_ostream) << to_init_value(decl_name, *decl->type());
            }
            (*m_ostream) << ";" << endl;
        }

        if (auto ctor = _node.constructor())
        {
            (*m_ostream) << "Ctor_" << translation.name << "(&tmp, state";
            for (auto decl : ctor->parameters())
            {
                (*m_ostream) << ", " << decl->name();
            }
            (*m_ostream) << ");" << endl;
        }

        (*m_ostream) << "return tmp;" << endl;
        (*m_ostream) << "}" << endl;
    }

    return true;
}

bool FunctionConverter::visit(StructDefinition const& _node)
{
    vector<ASTPointer<VariableDeclaration>> initializable_members;
    vector<ASTPointer<VariableDeclaration>> unitializable_members;
    for (auto const& member : _node.members())
    {
        if (is_basic_type(*member->type()))
        {
            initializable_members.push_back(member);
        }
        else
        {
            unitializable_members.push_back(member);
        }
    }

    auto translation = m_converter.translate(_node);

    (*m_ostream) << translation.type << " Init_" << translation.name;
    print_args(initializable_members, nullptr, true);

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << translation.type << " tmp;" << endl;

        for (auto decl : initializable_members)
        {
            (*m_ostream) << "tmp.d_" << decl->name() << " = " << decl->name()
                         << ";" << endl;
        }
        for (auto decl : unitializable_members)
        {
            (*m_ostream) << "tmp.d_" << decl->name() << " = Init_"
                         << m_converter.translate(*decl).name << "();" << endl;
        }

        (*m_ostream) << "return tmp;" << endl;
        (*m_ostream) << "}" << endl;
    }

    (*m_ostream) << translation.type << " ND_" << translation.name << "()";

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << translation.type << " tmp;" << endl;

        for (auto decl : _node.members())
        {
            auto decl_name = m_converter.translate(*decl).name;
            (*m_ostream) << "tmp.d_" << decl->name() << " = "
                         << to_nd_value(decl_name, *decl->type()) << ";" << endl;
        }

        (*m_ostream) << "return tmp;" << endl;
        (*m_ostream) << "}" << endl;
    }

    return true;
}

bool FunctionConverter::visit(FunctionDefinition const& _node)
{
    auto translation = m_converter.translate(_node);
    (*m_ostream) << translation.type << " " << translation.name;

    bool is_mutable = _node.stateMutability() != StateMutability::Pure;
    print_args(_node.parameters(), is_mutable ? _node.scope() : nullptr, false);

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl;
        BlockConverter(_node, m_converter).print(*m_ostream);
        (*m_ostream) << endl;
    }

    return false;
}

bool FunctionConverter::visit(ModifierDefinition const& _node)
{
    auto translation = m_converter.translate(_node);

    (*m_ostream) << translation.type << " " << translation.name;
    print_args(_node.parameters(), _node.scope(), false);
    (*m_ostream) << ";" << endl;

    return false;
}

bool FunctionConverter::visit(Mapping const& _node)
{
    Translation map_trans = m_converter.translate(_node);
    Translation key_trans = m_converter.translate(_node.keyType());
    Translation val_trans = m_converter.translate(_node.valueType());

    auto const& k_type = *_node.keyType().annotation().type;
    auto const& v_type = *_node.valueType().annotation().type;

    (*m_ostream) << map_trans.type << " " << "Init_" << map_trans.name << "()";

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << map_trans.type << " tmp;" << endl;
        (*m_ostream) << "tmp.m_set = 0;" << endl;
        (*m_ostream) << "tmp.m_curr = " << to_init_value(key_trans.name, k_type)
                     << ";" << endl;
        (*m_ostream) << "tmp.d_ = " << to_init_value(val_trans.name, v_type)
                     << ";" << endl;
        (*m_ostream) << "tmp.d_nd = " << to_init_value(val_trans.name, v_type)
                     << ";" << endl;
        (*m_ostream) << "return tmp;" << endl;
        (*m_ostream) << "}" << endl;
    }

    (*m_ostream) << map_trans.type << " " << "ND_" << map_trans.name << "()";

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << map_trans.type << " tmp;" << endl;
        (*m_ostream) << "tmp.m_set = ND_Init_Val();" << endl;
        (*m_ostream) << "tmp.m_curr = " << to_nd_value(key_trans.name, k_type)
                     << ";" << endl;
        (*m_ostream) << "tmp.d_ = " << to_nd_value(val_trans.name, v_type)
                     << ";" << endl;
        (*m_ostream) << "tmp.d_nd = " << to_init_value(val_trans.name, v_type)
                     << ";" << endl;
        (*m_ostream) << "return tmp;" << endl;
        (*m_ostream) << "}" << endl;
    }

    (*m_ostream) << val_trans.type << " Read_" << map_trans.name << "("
                 << map_trans.type << " *a, " << key_trans.type << " idx)";

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << "if (a->m_set == 0) { a->m_curr = idx; a->m_set = 1; }"
                     << endl;
        (*m_ostream) << "if (idx != a->m_curr) return "
                     << to_nd_value(val_trans.name, v_type) << ";" << endl;
        (*m_ostream) << "return a->d_;" << endl;
        (*m_ostream) << "}" << endl;
    }

    (*m_ostream) << "void Write_" << map_trans.name << "("
                 << map_trans.type << " *a, " << key_trans.type << " idx, "
                 << val_trans.type << " d)";

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << "if (a->m_set == 0) { a->m_curr = idx; a->m_set = 1; }"
                     << endl;
        (*m_ostream) << "if (idx == a->m_curr) { a->d_ = d; }" << endl;
        (*m_ostream) << "}" << endl;
    }

    (*m_ostream) << val_trans.type << " *Ref_" << map_trans.name << "("
                 << map_trans.type << " *a, " << key_trans.type << " idx)";

    if (m_forward_declare)
    {
        (*m_ostream) << ";" << endl;
    }
    else
    {
        (*m_ostream) << endl << "{" << endl;
        (*m_ostream) << "if (a->m_set == 0) { a->m_curr = idx; a->m_set = 1; }"
                     << endl;
        (*m_ostream) << "if (idx != a->m_curr)" << endl;
        (*m_ostream) << "{" << endl;
        (*m_ostream) << "a->d_nd = " << to_nd_value(val_trans.name, v_type)
                     << ";" << endl;
        (*m_ostream) << "return &a->d_nd;" << endl;
        (*m_ostream) << "}" << endl;
        (*m_ostream) << "return &a->d_;" << endl;
        (*m_ostream) << "}" << endl;
    }

    return true;
}

// -------------------------------------------------------------------------- //

bool FunctionConverter::is_basic_type(Type const& _type)
{
    switch (_type.category())
    {
        case Type::Category::Address:
        case Type::Category::Integer:
        case Type::Category::RationalNumber:
        case Type::Category::Bool:
        case Type::Category::FixedPoint:
        case Type::Category::Enum:
            return true;
        default:
            return false;
    }
}

string FunctionConverter::to_init_value(string _name, Type const& _type)
{
    return (is_basic_type(_type) ? "0" : "Init_" + _name + "()");
}

string FunctionConverter::to_nd_value(string _name, Type const& _type)
{
    return (is_basic_type(_type) ? "ND_Init_Val()" : "ND_" + _name + "()");
}

// -------------------------------------------------------------------------- //

void FunctionConverter::print_args(
    vector<ASTPointer<VariableDeclaration>> const& _args,
    ASTNode const* _scope,
    bool _default_to_zero
)
{
    (*m_ostream) << "(";

    auto contract_scope = dynamic_cast<ContractDefinition const*>(_scope);
    if (contract_scope)
    {
        auto type = m_converter.translate(*contract_scope).type;
        (*m_ostream) << type << " *self, struct CallState *state";
    }

    for (unsigned int idx = 0; idx < _args.size(); ++idx)
    {
        if (contract_scope || idx > 0)
        {
            (*m_ostream) << ", ";
        }

        auto const& arg = *_args[idx];
        (*m_ostream) << m_converter.translate(arg).type << " " << arg.name();

        if (_default_to_zero)
        {
            (*m_ostream) << " = 0";
        }
    }

    (*m_ostream) << ")";
}

// -------------------------------------------------------------------------- //

}
}
}
