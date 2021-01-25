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
class FlatContract;
class FlatModel;

// -------------------------------------------------------------------------- //

class ContractRvAnalyzer : public ASTConstVisitor
{
public:
    // Given _func returns a contract reference, this analyzer will aggregate
    // all possible return values for _func in contract _src.
    ContractRvAnalyzer(
        std::shared_ptr<FlatContract> _src,
        std::shared_ptr<FlatModel const> _model,
        std::shared_ptr<AllocationGraph const> _allocation_graph,
        FunctionDefinition const& _func
    );

    // Given _func is a library method which returns a contract, this analyzer
    // will aggregate all possible return values for _func.
    ContractRvAnalyzer(
        std::shared_ptr<FlatModel const> _model,
        std::shared_ptr<AllocationGraph const> _allocation_graph,
        FunctionDefinition const& _func
    );

    // Produces a union of all types returned by reference.
    std::set<std::shared_ptr<FlatContract>> internals() const;

    // Produces a union of all types that reference arbitrary addresses.
    std::set<std::shared_ptr<FlatContract>> externals() const;

    // A list of external calls which can be returned by the method.
    using Key = std::pair<std::shared_ptr<FlatContract>, FunctionDefinition const*>;
    std::set<Key> dependencies() const;

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
    std::set<std::shared_ptr<FlatContract>> m_internal_refs;
    std::set<std::shared_ptr<FlatContract>> m_external_refs;
    std::set<Key> m_procedural_calls;

    std::shared_ptr<FlatContract> m_src;

    bool m_is_in_return = false;

    std::shared_ptr<FlatModel const> m_model;
    std::shared_ptr<AllocationGraph const> m_allocation_graph;

    FunctionDefinition const& m_func;

    // Assumes _ref is the current AST node. If _ref is a feasible return value
    // it is registered as a potential return type.
    void record_ref(Expression const& _ref);
};

// -------------------------------------------------------------------------- //

class ContractRvLookup
{
public:
    // Extracts the true return value for each contract typed method in _model,
    // using the allocation records in _allocation_graph. 
    ContractRvLookup(
        std::shared_ptr<FlatModel const> _model,
        std::shared_ptr<AllocationGraph const> _allocation_graph
    );

    using Key = ContractRvAnalyzer::Key;
    std::map<Key, std::shared_ptr<ContractRvAnalyzer>> registry;

private:
    std::shared_ptr<FlatModel const> m_model;
    std::shared_ptr<AllocationGraph const> m_allocation_graph;

    //
    Key
    record(std::shared_ptr<FlatContract> _src, FunctionDefinition const& _func);

    //
    bool check_method_rv(FunctionDefinition const& _func);
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
    // using the allocation records in _allocation_graph. 
    ContractExpressionAnalyzer(
        std::shared_ptr<FlatModel const> _model,
        std::shared_ptr<AllocationGraph const> _allocation_graph
    );

    // Returns a more precise contract type for _expr. This takes into account
    // upcasting. Throws if the expression cannot be mapped to a recorded
    // declaration.
    //
    // If the expression appears within a contract, that contract is given by
    // _ctx.
    //
    // Valid types for _expr include MemberAccess, Identifier, and FunctionCall.
    std::shared_ptr<FlatContract>
    resolve(Expression const& _expr, std::shared_ptr<FlatContract> _ctx) const;

private:
    std::shared_ptr<FlatModel const> m_model;
    std::shared_ptr<AllocationGraph const> m_allocation_graph;

    std::map<ContractRvLookup::Key, std::shared_ptr<FlatContract>> m_rv_types;
};

// -------------------------------------------------------------------------- //

}
}
}
