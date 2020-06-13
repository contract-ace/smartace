#include <libsolidity/modelcheck/model/ADT.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/AllocationSites.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
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
    std::shared_ptr<AnalysisStack> _stack,
    bool _add_sums,
    size_t _map_k,
    bool _forward_declare
): M_AST(_ast)
 , M_ADD_SUMS(_add_sums)
 , M_MAP_K(_map_k)
 , M_FORWARD_DECLARE(_forward_declare)
 , m_stack(_stack)
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
                string const TYPE_NAME = TypeAnalyzer::get_simple_ctype(*TYPE);
                string const NAME = ContractUtilities::address_member();

                fields->push_back(make_shared<CVarDecl>(TYPE_NAME, NAME));
            }
            {
                TypePointer TYPE = ContractUtilities::balance_type();
                string const TYPE_NAME = TypeAnalyzer::get_simple_ctype(*TYPE);
                string const NAME = ContractUtilities::balance_member();

                fields->push_back(make_shared<CVarDecl>(TYPE_NAME, NAME));
            }

            auto flat_view = m_stack->model()->get(_node);
            for (auto decl : flat_view->state_variables())
            {
                string type;

                // TODO: flat map to pre-compute the category.
                auto const CATEGORY = decl->annotation().type->category();
                if (CATEGORY == Type::Category::Contract)
                {
                    auto const& ref = m_stack->allocations()->specialize(*decl);
                    type = m_stack->types()->get_type(ref);
                }
                else
                {
                    type = m_stack->types()->get_type(*decl);
                }

                string const NAME = VariableScopeResolver::rewrite(
                    decl->name(), false, VarContext::STRUCT
                );

                fields->push_back(make_shared<CVarDecl>(type, NAME));
            }
        }

        CStructDef contract(m_stack->types()->get_name(_node), move(fields));
        (*m_ostream) << contract;
    }

    return false;
}

// -------------------------------------------------------------------------- //

bool ADTConverter::visit(Mapping const& _node)
{
    MapGenerator mapgen(_node, M_ADD_SUMS, M_MAP_K, *m_stack->types());
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
            string const TYPE = m_stack->types()->get_type(*decl);
            string const NAME = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            fields->push_back(make_shared<CVarDecl>(TYPE, NAME));
        }
    }

    CStructDef structure(m_stack->types()->get_name(_node), move(fields));
    (*m_ostream) << structure;
}

// -------------------------------------------------------------------------- //

}
}
}
