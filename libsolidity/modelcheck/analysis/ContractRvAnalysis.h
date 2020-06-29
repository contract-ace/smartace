/**
 * Thee analysis tools build on the allocation graph to determine the true 
 * return type of each contract-typed function.
 * 
 * @date 2020
 */

#include <libsolidity/ast/ASTVisitor.h>

#include <map>
#include <memory>
#include <set>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AllocationGraph;
class FlatModel;

// -------------------------------------------------------------------------- //

class ContractRvAnalyzer : public ASTConstVisitor
{
public:
    // Given _func returns a contract method, this analyzer will aggregate all
    // possible return values for _func in contract _src.
    ContractRvAnalyzer(
        ContractDefinition const& _src,
        std::shared_ptr<AllocationGraph const> _allocation_graph,
        FunctionDefinition const& _func
    );

    // Produces a union of all types returned by reference.
    std::set<ContractDefinition const*> internals() const;

    // Produces a union of all types which reference arbitrary addresses.
    std::set<ContractDefinition const*> externals() const;

    // A list of external calls which can be returned by the method.
    std::set<FunctionDefinition const*> dependencies() const;

protected:
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(Conditional const& _node) override;
	bool visit(Return const& _node) override;
	bool visit(Assignment const& _node) override;
	bool visit(FunctionCall const& _node) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(IndexAccess const&) override;
	bool visit(Identifier const& _node) override;

private:
    std::set<ContractDefinition const*> m_internal_refs;
    std::set<ContractDefinition const*> m_external_refs;
    std::set<FunctionDefinition const*> m_procedural_calls;

    ContractDefinition const* m_src;

    bool m_is_in_return;

    std::shared_ptr<AllocationGraph const> m_allocation_graph;

    // Assumes _ref is the current AST node. If _ref is a feasible return value
    // it is registered as a potential return type.
    void record_ref(Expression const& _ref);
};

// -------------------------------------------------------------------------- //

/**
 * An analyzer used to collect the return value types for each contract-typed
 * method. This data is then used to build a unified mapping from expressions to
 * contract types. 
 */
class ContractExpressionAnalyzer
{
public:
    // Extracts the true return value for each contract typed method in _model,
    // using the allocation records in _alloc_graph. 
    ContractExpressionAnalyzer(
        FlatModel const& _model,
        std::shared_ptr<AllocationGraph const> _alloc_graph
    );

    // Returns a more precise contract type for _expr. This takes into account
    // upcasting. Throws if the expression cannot be mapped to a recorded
    // declaration.
    //
    // Valid types for _expr include MemberAccess, Identifier, and FunctionCall.
    ContractDefinition const& resolve(Expression const& _expr) const;

private:
    std::shared_ptr<AllocationGraph const> m_allocation_graph;
};

// -------------------------------------------------------------------------- //

}
}
}
