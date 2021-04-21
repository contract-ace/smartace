#include <libsolidity/modelcheck/analysis/CallState.h>

#include <libsolidity/modelcheck/analysis/CallGraph.h>
#include <libsolidity/modelcheck/analysis/FunctionCall.h>
#include <libsolidity/modelcheck/analysis/Primitives.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Ether.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>

#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CallState::CallState(CallGraph const& _graph, bool _escalate_reqs)
 : m_escalates_reqs(_escalate_reqs)
{
    for (auto call : _graph.executed_code())
    {
        if (call->isPayable())
        {
            m_uses_pay = true;
        }
        call->accept(*this);
    }

    // TODO: can we restrict this to only the fields we actually use?
    add_field(CallStateUtilities::Field::Sender);
    add_field(CallStateUtilities::Field::Value);
    add_field(CallStateUtilities::Field::Block);
    add_field(CallStateUtilities::Field::Timestamp);
    add_field(CallStateUtilities::Field::Paid);
    add_field(CallStateUtilities::Field::Origin);

    if (m_escalates_reqs)
    {
        add_field(CallStateUtilities::Field::ReqFail);
    }
}

void CallState::register_primitives(PrimitiveTypeGenerator& _gen) const
{
    for (auto field : m_field_order)
    {
        _gen.record_type(field.type);
    }
}

list<CallState::FieldData> const& CallState::order() const
{
    return m_field_order;
}

void CallState::push_state_to(CFuncCallBuilder & _builder) const
{
    CExprPtr sender;
    for (auto fld : order())
    {
        if (fld.field == CallStateUtilities::Field::Origin)
        {
            auto src_type = CallStateUtilities::Field::Sender;
            auto src_name = CallStateUtilities::get_name(src_type);
            _builder.push(make_shared<CIdentifier>(src_name, false));
        }
        else
        {
            _builder.push(make_shared<CIdentifier>(fld.name, false));
        }
    }
}

void CallState::compute_next_state_for(
    CFuncCallBuilder & _builder,
    bool _external,
    bool _for_contract,
    CExprPtr _value,
    CExprPtr _sender,
    CExprPtr _balance
) const
{
	auto self_id = make_shared<CIdentifier>("self", true);
	for (auto const& f : order())
	{
        if (!_for_contract && f.contract_only) continue;
		if (_external && f.field == CallStateUtilities::Field::Sender)
		{
            if (_sender)
            {
                _builder.push(_sender);
            }
            else
            {
			    string const ADDRESS = ContractUtilities::address_member();
			    _builder.push(make_shared<CMemberAccess>(self_id, ADDRESS));
            }
		}
		else if (_external && f.field == CallStateUtilities::Field::Value)
		{
			if (_value)
			{
				CFuncCallBuilder val_builder(Ether::PAY);
                if (_balance)
                {
                    val_builder.push(_balance);
                }
                else
                {
				    string const BAL = ContractUtilities::balance_member();
				    val_builder.push(make_shared<CReference>(self_id->access(BAL)));
                }
				val_builder.push(_value);
				_builder.push(val_builder.merge_and_pop());
			}
			else
			{
				_builder.push(TypeAnalyzer::init_val_by_simple_type(*f.type));
			}
		}
		else if (f.field == CallStateUtilities::Field::Paid)
		{
			auto const& PAID_RAW = _external ? Literals::ONE : Literals::ZERO;
			_builder.push(InitFunction::wrap(*f.type, PAID_RAW));
		}
		else
		{
			_builder.push(make_shared<CIdentifier>(f.name, false));
		}
	}
}

bool CallState::uses_send() const { return m_uses_send; }

bool CallState::uses_transfer() const { return m_uses_transfer; }

bool CallState::uses_pay() const { return m_uses_pay; }

bool CallState::escalate_requires() const { return m_escalates_reqs; }

void CallState::endVisit(FunctionCall const& _node)
{
    // The scan is in search of transfer and send which must be FunctionCall's.
	FunctionCallKind const KIND = _node.annotation().kind;
	if (KIND != FunctionCallKind::FunctionCall) return;

    // Otherwise checks if this is a contract method call.
	FunctionCallAnalyzer calldata(_node);
    if (calldata.classify() == FunctionCallAnalyzer::CallGroup::Send)
    {
        m_uses_send = true;
    }
    else if (calldata.classify() == FunctionCallAnalyzer::CallGroup::Transfer)
    {
        m_uses_transfer = true;
    }
}

void CallState::add_field(CallStateUtilities::Field _field)
{
    auto insert_res = m_recorded_fields.insert(_field);
    if (!insert_res.second) return;

    FieldData f;
    f.field = _field;
    f.name = CallStateUtilities::get_name(_field);
    f.type = CallStateUtilities::get_type(_field);
    f.type_name = TypeAnalyzer::get_simple_ctype(*f.type);
    f.contract_only = CallStateUtilities::is_contract_only(_field);
    m_field_order.push_back(move(f));
}

// -------------------------------------------------------------------------- //

}
}
}
