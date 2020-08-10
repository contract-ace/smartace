/**
 * Analyzes method calls to determine what we must model with respect to the
 * global state.
 * 
 * @todo(scottwe): could we dynamically restrict global variables while
 *                 maintaining soundless (relative to the MiniSol dialect).
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/utils/CallState.h>

#include <list>
#include <set>
#include <string>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallGraph;
class CFuncCallBuilder;
class PrimitiveTypeGenerator;

// -------------------------------------------------------------------------- //

/**
 * Provides an interface for aggregating call state data, and using it to
 * produce call state related abstractions.
 */
class CallState : public ASTConstVisitor
{
public:
    // Associates a field with its data.
    struct FieldData
    {
        CallStateUtilities::Field field;
        std::string name;
        TypePointer type;
        std::string type_name;
        bool contract_only;
    };

    // A sufficient state for each call is inferred from _graph. If the
    // _escalate_reqs flag is set, the call state will propogate the ReqFail
    // flag will be propogated.
    CallState(CallGraph const& _graph, bool _escalate_reqs);

    // Allows the CallState to pass dependencies to the primitive generator.
    void register_primitives(PrimitiveTypeGenerator& _gen) const;

    // Returns the order of fields in use.
    std::list<FieldData> const& order() const;

    // Appends all argument ID's to an argument list, in order.
    void push_state_to(CFuncCallBuilder & _builder) const;

    // Handles callstate updates for intermediate calls.
    void compute_next_state_for(
        CFuncCallBuilder & _builder,
        bool _external,
        bool _for_contract,
        CExprPtr _value
    ) const;

    // Returns true if send is required.
    bool uses_send() const;

    // Returns true if transfer is required.
    bool uses_transfer() const;

    // Returns true if methods are paid.
    bool uses_pay() const;

    // Returns true if requires should be escalated to assertions.
    bool escalate_requires() const;

protected:
    void endVisit(FunctionCall const& _node) override;

private:
    // Adds a new field (and its associated data) to the field list.
    void add_field(CallStateUtilities::Field _field);

    bool m_uses_send = false;
    bool m_uses_transfer = false;
    bool m_uses_pay = false;

    bool m_escalates_reqs = false;

    std::set<CallStateUtilities::Field> m_recorded_fields;
    std::list<FieldData> m_field_order;
};

// -------------------------------------------------------------------------- //

}
}
}
