/**
 * @date 2019
 * First-pass visitor for aggregating call state data. This data may then be
 * used to generate a call state abstraction for use by a C model.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/utils/CallState.h>
#include <list>
#include <ostream>
#include <set>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class PrimitiveTypeGenerator;
class CFuncCallBuilder;

/**
 * Provides an interface for aggregating call state data, and using it to produce
 * call state related abstractions.
 */
class CallState : public ASTConstVisitor
{
public:
    // Associates a field with its data.
    struct FieldData
    {
        CallStateUtilities::Field field;
        std::string name;
        std::string temp;
        TypePointer type;
        std::string tname;
    };

    CallState();

    // Aggregates data from an AST node.
    // TODO(scottwe: this is just a placeholder for now.
    void record(ASTNode const& _ast);

    // Prints an appropiate call state, along with the appropriate helpers, to
    // _stream. If _forward_declare is set, then the bodies are ellided.
    void print(std::ostream& _stream, bool _forward_declare) const;

    // Allows the CallState to pass dependencies to the primitive generator.
    void register_primitives(PrimitiveTypeGenerator& _gen) const;

    // Returns the order of fields in use.
    std::list<FieldData> const& order() const;

    // Appends all argument ID's to an argument list, in order.
    void push_state_to(CFuncCallBuilder & _builder) const;

private:
    // Adds a new field (and its associated data) to the field list.
    void add_field(CallStateUtilities::Field _field);

    std::set<CallStateUtilities::Field> m_recorded_fields;
    std::list<FieldData> m_field_order;
};

}
}
}
