/**
 * @date 2019
 * First-pass visitor for aggregating call state data. This data may then be
 * used to generate a call state abstraction for use by a C model.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <ostream>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class PrimitiveTypeGenerator;

/**
 * Provides an interface for aggregating call state data, and using it to produce
 * call state related abstractions.
 */
class CallState : public ASTConstVisitor
{
public:
    // Aggregates data from an AST node.
    // TODO(scottwe: this is just a placeholder for now.
    void record(ASTNode const& _ast);

    // Prints an appropiate call state, along with the appropriate helpers, to
    // _stream. If _forward_declare is set, then the bodies are ellided.
    void print(std::ostream& _stream, bool _forward_declare) const;

    // Allows the CallState to provide its requirements to the primitive
    // generator.
    void register_primitives(PrimitiveTypeGenerator& _gen) const;
};

}
}
}
