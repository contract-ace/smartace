#include <libsolidity/modelcheck/analysis/Mapping.h>

#include <libsolidity/modelcheck/utils/AST.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MappingExtractor::MappingExtractor(
    vector<ASTPointer<VariableDeclaration>> _vars
)
{
    for (auto var : _vars) record(var.get());
}

void MappingExtractor::record(VariableDeclaration const* _var)
{
    _var->accept(*this);
}

list<Mapping const*> MappingExtractor::get() const { return m_mappings; }

bool MappingExtractor::visit(Mapping const& _node)
{
    m_mappings.push_back(&_node);
    return false;
}

// -------------------------------------------------------------------------- //

MapDeflate::Record MapDeflate::query(Mapping const& _map)
{
    auto flatmap = make_shared<FlatMap>();
    m_flatset[&_map] = flatmap;

    if (flatmap->name.empty())
    {
        flatmap->name = "Map_" + to_string(m_flatset.size());
        
        Mapping const* curpos = (&_map);
        while (curpos != nullptr)
        {
            flatmap->value_type = &curpos->valueType();
            flatmap->key_types.push_back(&curpos->keyType());
            curpos = dynamic_cast<Mapping const*>(flatmap->value_type);
        }
    }

    return flatmap;
}

MapDeflate::Record MapDeflate::resolve(Mapping const& _mapping) const
{
    auto const& record = m_flatset.find(&_mapping);
    if (record == m_flatset.end()) return nullptr;
    return record->second;
}

MapDeflate::Record MapDeflate::resolve(VariableDeclaration const& _decl) const
{
    auto const* mapping = dynamic_cast<Mapping*>(_decl.typeName());
    if (!mapping) return nullptr;
    return resolve(*mapping);
}

MapDeflate::Record MapDeflate::try_resolve(Mapping const& _mapping) const
{
    if (auto res = resolve(_mapping)) return res;
    throw runtime_error("MapDeflate used to resolve unknown declaration.");
}

MapDeflate::Record MapDeflate::try_resolve(VariableDeclaration const& _decl) const
{
    if (auto res = resolve(_decl)) return res;
    throw runtime_error("Unable to extract Mapping from IndexAccess");
}

// -------------------------------------------------------------------------- //

FlatIndex::FlatIndex(IndexAccess const& _root)
{
    _root.accept(*this);
}

list<Expression const*> const& FlatIndex::indices() const
{
    return m_indices;
}


Expression const& FlatIndex::base() const
{
    return (*m_base);
}

VariableDeclaration const& FlatIndex::decl() const
{
    return (*m_decl);
}

// -------------------------------------------------------------------------- //

bool FlatIndex::visit(Conditional const&)
{
    // TODO: this will require unified map types for each key/type pattern.
    throw runtime_error("Conditional map resolution not yet supported.");
}

bool FlatIndex::visit(MemberAccess const& _node)
{
    if (auto decl = member_access_to_decl(_node)) m_decl = decl;
    return false;
}

bool FlatIndex::visit(IndexAccess const& _node)
{
    m_indices.push_front(_node.indexExpression());
    if (m_indices.front() == nullptr)
    {
        throw runtime_error("Map access with null index.");
    }
    m_base = (&_node.baseExpression());
    m_base->accept(*this);
    return false;
}

bool FlatIndex::visit(Identifier const& _node)
{
    m_decl = dynamic_cast<VariableDeclaration const*>(
        _node.annotation().referencedDeclaration
    );
    return false;
}

// -------------------------------------------------------------------------- //

}
}
}
