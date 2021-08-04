/**
 * Utility to perform taint analysis with client variables.
 * 
 * @date 2021
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <map>
#include <memory>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Utility to extract assignment destination (wrt. taint analysis).
 */
class TaintDestination : public ASTConstVisitor
{
public:
    // Extracts destination from _expr.
    TaintDestination(Expression const& _expr);

    // Extracts the destination, or throws an exception.
    VariableDeclaration const& extract();

protected:
    bool visit(IndexAccess const& _node) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
    // Updates m_dest. If m_dest is set, throws.
    void update_dest(Declaration const* _ref);

    // First destination located.
    VariableDeclaration const* m_dest = nullptr;
};

// -------------------------------------------------------------------------- //

/**
 * Performs intraprocedural tain propogation for a single method. Tainted
 * sources are flagged before running the analysis.
 *
 * All analysis is flow-insensitive and field-insensitive.
 *
 * Analysis is very coase. For example, if `x = e`, and e contains tainted
 * variable y, then x is now tainted, regardless of interpretation of e.
 */
class TaintAnalysis : public ASTConstVisitor
{
public:
    // Analysis with tainted sources numbered 0 to (_sources - 1).
    TaintAnalysis(size_t _sources);

    // Marks a tainted source before running the analysis.
    void taint(VariableDeclaration const& _decl, size_t _i);

    // Runs taint analysis until a fixed point is reached.
    void run(FunctionDefinition const& _decl);

    // Returns the taint result for _decl.
    std::vector<bool> const& taint_for(VariableDeclaration const& _decl) const;

    // Returns the number of taitned sources.
    size_t source_count() const;

protected:
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(Assignment const& _node) override;
	bool visit(FunctionCall const&) override;
	bool visit(MemberAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
    // Moves taint from _decl to m_taintee.
    void propogate(Declaration const* _ref);

    // Applies all taint to m_taint due to inprecision.
    void propogate_unknown();

    // Default response for unknown variables.
    std::vector<bool> m_default_taint;

    // Set to false whenever the taint database is updated.
    bool m_changed = false;

    // The number of taited sources.
    size_t m_sources;

    // Maps each variable declaration to the inputs it has been tainted by.
    // Ex. If m_taint[v][i] is true, then variable v is tainted by input i.
    std::map<VariableDeclaration const*, std::vector<bool>> m_taint;

    // Current variable being tainted.
    VariableDeclaration const *m_taintee;
};

// -------------------------------------------------------------------------- //

/**
 * Performs taint analysis for PTGBuilder.
 */
class AbstractTaintPass : public ASTConstVisitor
{
public:
    // Returns the result.
    std::vector<bool> const& extract() const;

protected:
    // Perform analysis after setup.
    void setup(FunctionDefinition const& _func);

    // Intergration Interfaces.
    virtual size_t compute_source_ct(FunctionDefinition const& _func) const = 0;
    virtual void populate(
        FunctionDefinition const& _func, std::shared_ptr<TaintAnalysis> _data
    ) const = 0;

    // Interprocedural (sinks).
    bool visit(Return const& _node) override;
	bool visit(FunctionCall const& _node) override;

    // Intraprocedural (sinks).
	bool visit(UnaryOperation const& _node) override;
	bool visit(BinaryOperation const& _node) override;
	bool visit(IndexAccess const& _node) override;

    // Program state (sinks); Inputs (sources);
	bool visit(MemberAccess const& _node) override;
	bool visit(Identifier const& _node) override;

private:
    // Processes a declaration.
    void process_declaration(Declaration const* _ref);

    // The taint data propogated to the sinks.
    std::shared_ptr<TaintAnalysis> m_taint_data;

    // If true, the program is in a location that can use tainted sources.
    bool m_in_sink = false;

    // Tainted sources that reach sinks.
    std::vector<bool> m_reached_sinks;
};

// -------------------------------------------------------------------------- //

/**
 * Performs client taint analysis for PTGBuilder.
 */
class ClientTaintPass : public AbstractTaintPass
{
public:
    // Performs PTG analysis for _func.
    ClientTaintPass(FunctionDefinition const& _func);

protected:
    // Overrides.
    size_t compute_source_ct(FunctionDefinition const& _func) const override;
    void populate(
        FunctionDefinition const& _func, std::shared_ptr<TaintAnalysis> _data
    ) const override;
};

// -------------------------------------------------------------------------- //

/**
 * Performs role taint analysis for PTGBuilder.
 */
class RoleTaintPass : public AbstractTaintPass
{
public:
    using Roles = std::vector<VariableDeclaration const*>;

    // Performs PTG analysis for _func.
    RoleTaintPass(FunctionDefinition const& _func, Roles _roles);

protected:
    // Overrides.
    size_t compute_source_ct(FunctionDefinition const&) const override;
    void populate(
        FunctionDefinition const&, std::shared_ptr<TaintAnalysis> _data
    ) const override;

protected:
    // Stores copy of roles.
    Roles m_roles;
};

// -------------------------------------------------------------------------- //

}
}
}
