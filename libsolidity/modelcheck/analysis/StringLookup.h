/**
 * Handles the encoding of string literals.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>

#include <cstdint>
#include <map>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallGraph;
class FlatModel;

// -------------------------------------------------------------------------- //

/**
 * Generates a mapping from string literals to model values. This encoding is
 * deterministic (given the AST of a smart contract) and ensures that there are
 * never collisions.
 */
class StringLookup : public ASTConstVisitor
{
public:
    // Records all strings in method bodies and state variable declarations.
    StringLookup(FlatModel const& _model, CallGraph const& _graph);

    // Returns the value that corresponds to a string literal. Trhows if the
    // literal is not a string.
    uint32_t lookup(Literal const& _node) const;

protected:
    void endVisit(Literal const& _node) override;

private:
    std::map<std::string, uint32_t> m_registry;
    uint32_t m_curr_index;
};

// -------------------------------------------------------------------------- //

}
}
}