/**
 * Determines the call graph for a model surface.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <map>
#include <memory>
#include <set>
#include <stdexcept>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AllocationGraph;
class FlatContract;
class FlatModel;
class FunctionCallAnalyzer;

// -------------------------------------------------------------------------- //

/**
 * This assumes that the call _func was made from within a method of _scope. The
 * contract against which to call _func is then resolved using the _alloc_graph
 * and _model.
 */
std::shared_ptr<FlatContract> devirtualize(
    std::shared_ptr<FlatContract> _scope,
    AllocationGraph const& _alloc_graph,
    FlatModel const& _model,
    FunctionCallAnalyzer const& _func
);

// -------------------------------------------------------------------------- //

/**
 * Generic implementation of a digraph. Here Vertex is the type of each vertex.
 */
template <typename Vertex>
class Digraph
{
public:
    // Returns true if a vertex exists at _v.
    bool has_vertex(Vertex _v) const
    {
        return (m_vertices.find(_v) != m_vertices.end());
    }

    // Returns true if an edge exists from _src to _dst.
    bool has_edge(Vertex _src, Vertex _dst) const
    {
        auto match = m_edges.find(_src);
        if (match != m_edges.end())
        {
            return (match->second.find(_dst) != match->second.end());
        }
        return false;
    }

    // Adds a vertex at _v.
    void add_vertex(Vertex _v) { m_vertices.insert(_v); }

    // Creates an edge from _src to _dst, under the assumption that both _src
    // and _dst are edges.
    void add_edge(Vertex _src, Vertex _dst)
    {
        if (!has_vertex(_src))
        {
            throw std::invalid_argument("Digraph::add_edge expects known _src");
        }
        if (!has_vertex(_dst))
        {
            throw std::invalid_argument("Digraph::add_edge expects known _dst");
        }
        m_edges[_src].insert(_dst);
    }

    // Returns all vertices.
    std::set<Vertex> vertices() const { return m_vertices; }

    // Returns the neighbours of a vertex.
    std::set<Vertex> neighbours(Vertex _src) const
    {
        auto match = m_edges.find(_src);
        if (match != m_edges.end())
        {
            return match->second;
        }
        return {};
    }

private:
    std::set<Vertex> m_vertices;
    std::map<Vertex, std::set<Vertex>> m_edges;
};

/**
 * Extends a digraph with labeled edges. By default edges have no labels.
 */
template <typename Vertex, typename Label>
class LabeledDigraph : public Digraph<Vertex>
{
public:
    // Labeles the edge from _src to _dst by _label. If the edge does not exist,
    // the label is cached.
    void label_edge(Vertex _src, Vertex _dst, Label _label)
    {
        m_labels[std::make_pair(_src, _dst)].insert(_label);
    }

    // Returns the label from _src to _dst. If no edge exists, the default value
    // is returned.
    std::set<Label> label_of(Vertex _src, Vertex _dst)
    {
        return m_labels[std::make_pair(_src,_dst)];
    }

private:
    std::map<std::pair<Vertex, Vertex>, std::set<Label>> m_labels;
};

// -------------------------------------------------------------------------- //

enum class CallTypes { Super, External, Alloc };

/**
 * Wrapper to the algorithms required to generate a call graph from a flat
 * Solidity model.
 */
class CallGraphBuilder : public ASTConstVisitor
{
public:
    using Graph = LabeledDigraph<FunctionDefinition const*, CallTypes>;

    // The call graph will downcast all contract variables through the use of
    // _alloc_graph.
    CallGraphBuilder(std::shared_ptr<AllocationGraph> _alloc_graph);

    // Computes a call graph for _model.
    std::shared_ptr<Graph> build(std::shared_ptr<FlatModel> _model);

protected:
    bool visit(FunctionDefinition const& _node) override;

    void endVisit(FunctionCall const& _node) override;

private:
    std::shared_ptr<AllocationGraph> m_alloc_graph;
    std::shared_ptr<FlatModel> m_model;

    std::shared_ptr<Graph> m_graph;

    std::list<FunctionDefinition const*> m_stack;
    std::list<std::shared_ptr<FlatContract>> m_scope;
    std::set<CallTypes> m_labels;
};

// -------------------------------------------------------------------------- //

}
}
}
