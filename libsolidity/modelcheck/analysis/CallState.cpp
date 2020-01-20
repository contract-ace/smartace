/**
 * @date 2019
 * First-pass visitor for generating the CallState of Solidity in C models,
 * which consist of the struct of CallState.
 */

#include <libsolidity/modelcheck/analysis/CallState.h>

#include <libsolidity/modelcheck/analysis/Primitives.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/General.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CallState::CallState()
{
    add_field(CallStateUtilities::Field::Sender);
    add_field(CallStateUtilities::Field::Value);
    add_field(CallStateUtilities::Field::Block);
    add_field(CallStateUtilities::Field::Timestamp);
    add_field(CallStateUtilities::Field::Paid);
    add_field(CallStateUtilities::Field::Origin);
}

// -------------------------------------------------------------------------- //

void CallState::record(ASTNode const& _ast)
{
    _ast.accept(*this);
}

// -------------------------------------------------------------------------- //

void CallState::print(std::ostream& _stream, bool _forward_declare) const
{
    auto const VALUE_T = TypeConverter::get_simple_ctype(
        *CallStateUtilities::get_type(CallStateUtilities::Field::Value)
    );
    auto const SENDER_T = TypeConverter::get_simple_ctype(
        *CallStateUtilities::get_type(CallStateUtilities::Field::Sender)
    );
    auto const PAID_T = TypeConverter::get_simple_ctype(
        *CallStateUtilities::get_type(CallStateUtilities::Field::Paid)
    );

    shared_ptr<CBlock> throw_pay_body;
    shared_ptr<CBlock> nothrow_pay_body;
    shared_ptr<CBlock> pay_by_val_body;

    auto const dst_var = make_shared<CVarDecl>(SENDER_T, "dst");
    auto const amt_var = make_shared<CVarDecl>(VALUE_T, "amt");
    auto const bal_var = make_shared<CVarDecl>(VALUE_T, "bal", true);

    if (!_forward_declare)
    {
        string ndmsg("Return value for send.");
        auto nd_result = make_shared<CFuncCall>(
            "rt_nd_byte", CArgList{ make_shared<CStringLiteral>(ndmsg) }
        );
        auto bal_cond = make_shared<CBinaryOp>(
            bal_var->access("v"), ">=", amt_var->access("v")
        );
        auto bal_check = make_shared<CFuncCall>(
            "sol_require", CArgList{ bal_cond, Literals::ZERO }
        );
        auto bal_change = make_shared<CBinaryOp>(
            bal_var->access("v"), "-=", amt_var->access("v")
        );

        throw_pay_body = make_shared<CBlock>(CBlockList{
            // Note: if throw were to fail the transaction is unobserved
            bal_check->stmt(),
            bal_change->stmt()
            // TODO: when address is in range
        });

        nothrow_pay_body = make_shared<CBlock>(CBlockList{
            make_shared<CIf>(
                bal_cond,
                make_shared<CBlock>(CBlockList{
                    bal_change->stmt(),
                    make_shared<CReturn>(nd_result)
                }),
                make_shared<CReturn>(Literals::ZERO)
            )
            // TODO: when address is in range
        });

        pay_by_val_body = make_shared<CBlock>(CBlockList{
            bal_check->stmt(),
            bal_change->stmt(),
            make_shared<CReturn>(amt_var->id())
        });
    }

    CFuncDef pay(
        make_shared<CVarDecl>("void", "_pay"), CParams{
            bal_var, dst_var, amt_var
        }, move(throw_pay_body)
    );

    CFuncDef pay_use_rv(
        make_shared<CVarDecl>("uint8_t", "_pay_use_rv"), CParams{
            bal_var, dst_var, amt_var
        }, move(nothrow_pay_body)
    );

    CFuncDef pay_by_value(
        make_shared<CVarDecl>(VALUE_T, "_pay_by_val"), CParams{
            bal_var, amt_var
        }, move(pay_by_val_body)
    );

    _stream << pay << pay_use_rv << pay_by_value;
}

// -------------------------------------------------------------------------- //

void CallState::register_primitives(PrimitiveTypeGenerator& _gen) const
{
    for (auto field : m_field_order)
    {
        _gen.record_type(field.type);
    }
}

// -------------------------------------------------------------------------- //

list<CallState::FieldData> const& CallState::order() const
{
    return m_field_order;
}

// -------------------------------------------------------------------------- //

void CallState::push_state_to(CFuncCallBuilder & _builder) const
{
    for (auto fld : order())
    {
        _builder.push(make_shared<CIdentifier>(fld.name, false));
    }
}

// -------------------------------------------------------------------------- //

void CallState::compute_next_state_for(
    CFuncCallBuilder & _builder, bool _external, CExprPtr _value
) const
{
	auto self_id = make_shared<CIdentifier>("self", true);
	for (auto const& f : order())
	{
		if (_external && f.field == CallStateUtilities::Field::Sender)
		{
			string const ADDRESS = ContractUtilities::address_member();
			_builder.push(make_shared<CMemberAccess>(self_id, ADDRESS));
		}
		else if (_external && f.field == CallStateUtilities::Field::Value)
		{
			if (_value)
			{
				string const BAL = ContractUtilities::balance_member();
				CFuncCallBuilder val_builder("_pay_by_val");
				val_builder.push(make_shared<CReference>(self_id->access(BAL)));
				val_builder.push(_value);
				_builder.push(val_builder.merge_and_pop());
			}
			else
			{
				_builder.push(TypeConverter::init_val_by_simple_type(*f.type));
			}
		}
		else if (f.field == CallStateUtilities::Field::Paid)
		{
			auto const& PAID_RAW = _external ? Literals::ONE : Literals::ZERO;
			_builder.push(FunctionUtilities::try_to_wrap(*f.type, PAID_RAW));
		}
		else
		{
			_builder.push(make_shared<CIdentifier>(f.name, false));
		}
	}
}

// -------------------------------------------------------------------------- //

void CallState::add_field(CallStateUtilities::Field _field)
{
    auto insert_res = m_recorded_fields.insert(_field);
    if (!insert_res.second) return;

    FieldData f;
    f.field = _field;
    f.name = CallStateUtilities::get_name(_field);
    f.temp = f.name + "_tmp";
    f.type = CallStateUtilities::get_type(_field);
    f.tname = TypeConverter::get_simple_ctype(*f.type);
    m_field_order.push_back(move(f));
}

// -------------------------------------------------------------------------- //

}
}
}
