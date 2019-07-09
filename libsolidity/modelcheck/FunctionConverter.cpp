/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/BlockConverter.h>
#include <libsolidity/modelcheck/ExpressionConverter.h>
#include <libsolidity/modelcheck/FunctionConverter.h>
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
    m_ostream = &_stream;
    m_ast->accept(*this);
    m_ostream = nullptr;
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
        (*m_ostream) << endl << "{" << endl
                     << translation.type << " tmp;" << endl;

        for (auto decl : _node.stateVariables())
        {
            auto decl_translation = m_converter.translate(*decl);

            (*m_ostream) << "tmp.d_" << decl->name() << " = ";
            if (decl->value())
            {
                ExpressionConverter(*decl->value(), {}, {}).print(*m_ostream);
            }
            else
            {
                switch (decl->type()->category())
                {
                case Type::Category::Address:
                case Type::Category::Integer:
                case Type::Category::RationalNumber:
                case Type::Category::Bool:
                case Type::Category::FixedPoint:
                case Type::Category::Enum:
                    (*m_ostream) << "0";
                    break;
                default:
                    (*m_ostream) << "Init_" << decl_translation.name << "()";
                    break;
                }
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

        (*m_ostream) << "return tmp;" << endl
                     << "}" << endl;
    }

    return true;
}

bool FunctionConverter::visit(StructDefinition const& _node)
{
    vector<ASTPointer<VariableDeclaration>> initializable_members;
    vector<ASTPointer<VariableDeclaration>> unitializable_members;
    for (auto const& member : _node.members())
    {
        switch (member->type()->category())
        {
        case Type::Category::Address:
        case Type::Category::Integer:
        case Type::Category::RationalNumber:
        case Type::Category::Bool:
        case Type::Category::FixedPoint:
        case Type::Category::Enum:
            initializable_members.push_back(member);
            break;
        default:
            unitializable_members.push_back(member);
            break;
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
        (*m_ostream) << endl << "{" << endl
                     << translation.type << " tmp;" << endl;

        for (auto decl : initializable_members)
        {
            (*m_ostream) << "tmp.d_" << decl->name() << " = " << decl->name()
                         << ";" << endl;
        }
        for (auto decl : unitializable_members)
        {
            auto decl_translation = m_converter.translate(*decl);
            (*m_ostream) << "tmp,d_" << decl->name() << " = "
                         << "Init_" << decl_translation.name << "();" << endl;
        }

        (*m_ostream) << "return tmp;" << endl
                     << "}" << endl;
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
    Translation map_translation = m_converter.translate(_node);

    string key_type = m_converter.translate(_node.keyType()).type;
    string val_type = m_converter.translate(_node.valueType()).type;

    (*m_ostream) << map_translation.type << " "
                 << "Init" << "_" << map_translation.name << "();" << endl;

    (*m_ostream) << val_type << " "
                 << "Read_" << map_translation.name
                 << "(" << map_translation.type << " *a, "
                        << key_type << " idx"
                 << ");"
                 << endl;

    (*m_ostream) << "void "
                 << "Write_" << map_translation.name
                 << "(" << map_translation.type << " *a, "
                        << key_type << " idx, "
                        << val_type << " d"
                 << ");"
                 << endl;

    (*m_ostream) << val_type << " *"
                 << "Ref_" << map_translation.name
                 << "(" << map_translation.type << " *a, "
                        << key_type << " idx"
                 << ");"
                 << endl;

    return true;
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
        Translation type_translation = m_converter.translate(arg);
        (*m_ostream) << type_translation.type << " " << arg.name();

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
