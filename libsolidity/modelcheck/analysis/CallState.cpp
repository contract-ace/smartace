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

    shared_ptr<CBlock> pay_body;
    shared_ptr<CBlock> pay_by_val_body;

    auto const dst_var = make_shared<CVarDecl>(SENDER_T, "dst");
    auto const amt_var = make_shared<CVarDecl>(VALUE_T, "amt");
    auto const bal_var = make_shared<CVarDecl>(VALUE_T, "bal", true);

    if (_forward_declare)
    {
        auto bal_cond = make_shared<CBinaryOp>(
            bal_var->access("v"), ">=", amt_var->access("v")
        );
        auto bal_check = make_shared<CFuncCall>(
            "sol_require", CArgList{ bal_cond, Literals::ZERO }
        );
        auto bal_change = make_shared<CBinaryOp>(
            bal_var->access("v"), "-=", amt_var->access("v")
        );

        pay_body = make_shared<CBlock>(CBlockList{});

        pay_by_val_body = make_shared<CBlock>(CBlockList{
            bal_check->stmt(),
            bal_change->stmt(),
            make_shared<CReturn>(amt_var->id())
        });
    }

    CFuncDef pay(
        make_shared<CVarDecl>("void", "_pay"), CParams{
            dst_var, amt_var
        }, move(pay_body)
    );

    CFuncDef pay_by_value(
        make_shared<CVarDecl>(VALUE_T, "_pay_by_val"), CParams{
            bal_var, amt_var
        }, move(pay_by_val_body)
    );

    _stream << pay << pay_by_value;
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
