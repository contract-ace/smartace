/**
 * @date 2019
 * First-pass visitor for converting Solidity AST's to models in C.
 */

#include <libsolidity/modelcheck/model/ADT.h>

#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/model/Mapping.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/General.h>

#include <set>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

ADTConverter::ADTConverter(
    ASTNode const& _ast,
    NewCallGraph const& _newcalls,
    TypeConverter const& _converter,
    size_t _map_k,
    bool _forward_declare
): M_AST(_ast)
 , M_CALLGRAPH(_newcalls)
 , M_CONVERTER(_converter)
 , M_MAP_K(_map_k)
 , M_FORWARD_DECLARE(_forward_declare)
{
}

void ADTConverter::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    M_AST.accept(*this);
}

// -------------------------------------------------------------------------- //

bool ADTConverter::visit(ContractDefinition const& _node)
{
    if (_node.isInterface()) return false;

    // TODO: this should accumulate, but right now is done per source unit.
    auto res = m_built.insert(&_node);
    if (!res.second) return false;

    for (auto dep : _node.annotation().contractDependencies) dep->accept(*this);
    for (auto structure : _node.definedStructs()) structure->accept(*this);
    for (auto decl : _node.stateVariables()) decl->accept(*this);

    if (!_node.isLibrary())
    {
        shared_ptr<CParams> fields;
        if (!M_FORWARD_DECLARE)
        {
            fields = make_shared<CParams>();
            {
                TypePointer TYPE = ContractUtilities::address_type();
                string const TYPE_NAME = TypeConverter::get_simple_ctype(*TYPE);
                string const NAME = ContractUtilities::address_member();

                fields->push_back(make_shared<CVarDecl>(TYPE_NAME, NAME));
            }
            {
                TypePointer TYPE = ContractUtilities::balance_type();
                string const TYPE_NAME = TypeConverter::get_simple_ctype(*TYPE);
                string const NAME = ContractUtilities::balance_member();

                fields->push_back(make_shared<CVarDecl>(TYPE_NAME, NAME));
            }

            set<string> vars;
            for (auto const* base : _node.annotation().linearizedBaseContracts)
            {
                for (auto decl : base->stateVariables())
                {
                    auto res = vars.insert(decl->name());
                    if (!res.second) break;

                    string type;
                    if (decl->annotation().type->category() == Type::Category::Contract)
                    {
                        type = M_CONVERTER.get_type(M_CALLGRAPH.specialize(*decl));
                    }
                    else
                    {
                        type = M_CONVERTER.get_type(*decl);
                    }

                    string const NAME = VariableScopeResolver::rewrite(
                        decl->name(), false, VarContext::STRUCT
                    );

                    fields->push_back(make_shared<CVarDecl>(type, NAME));
                }
            }
        }

        CStructDef contract(M_CONVERTER.get_name(_node), move(fields));
        (*m_ostream) << contract;
    }

    return false;
}

// -------------------------------------------------------------------------- //

bool ADTConverter::visit(Mapping const& _node)
{
    MapGenerator mapgen(_node, M_MAP_K, M_CONVERTER);
    (*m_ostream) << mapgen.declare(M_FORWARD_DECLARE);
    return false;
}

// -------------------------------------------------------------------------- //

void ADTConverter::endVisit(VariableDeclaration const& _node)
{
    auto const* pt = dynamic_cast<ContractType const*>(_node.annotation().type);
    if (pt)
    {
        pt->contractDefinition().accept(*this);
    }
}

void ADTConverter::endVisit(StructDefinition const& _node)
{
    shared_ptr<CParams> fields;

    if (!M_FORWARD_DECLARE)
    {
        fields = make_shared<CParams>();
        for (auto decl : _node.members())
        {
            string const TYPE = M_CONVERTER.get_type(*decl);
            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            fields->push_back(make_shared<CVarDecl>(TYPE, NAME));
        }
    }

    CStructDef structure(M_CONVERTER.get_name(_node), move(fields));
    (*m_ostream) << structure;
}

// -------------------------------------------------------------------------- //

}
}
}
