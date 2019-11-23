/**
 * @date 2019
 * First-pass visitor for generating the CallState of Solidity in C models,
 * which consist of the struct of CallState.
 */

#include <libsolidity/modelcheck/analysis/CallState.h>

#include <libsolidity/modelcheck/analysis/Primitives.h>
#include <libsolidity/modelcheck/analysis/Types.h>
#include <libsolidity/modelcheck/codegen/Details.h>
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
    add_field(CallStateUtilities::Field::Block);
    add_field(CallStateUtilities::Field::Sender);
    add_field(CallStateUtilities::Field::Value);
}

// -------------------------------------------------------------------------- //

void CallState::record(ASTNode const& _ast)
{
    _ast.accept(*this);
}

// -------------------------------------------------------------------------- //

void CallState::print(std::ostream& _stream, bool _forward_declare) const
{
    shared_ptr<CParams> cs_fields;
    shared_ptr<CBlock> pay_body;

    auto addr_type = TypeConverter::get_simple_ctype(
        AddressType(StateMutability::Payable)
    );
    auto uint256_type = TypeConverter::get_simple_ctype(IntegerType(256));

    if (_forward_declare)
    {
        pay_body = make_shared<CBlock>(CBlockList{});
    }

    CFuncDef pay(
        make_shared<CVarDecl>("void", "_pay"), CParams{
            make_shared<CVarDecl>(addr_type, "dst"),
            make_shared<CVarDecl>(uint256_type, "amt")
        }, move(pay_body)
    );

    _stream << pay;
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
