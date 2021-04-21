#include <libsolidity/modelcheck/model/Ether.h>

#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TightBundle.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/utils/CallState.h>
#include <libsolidity/modelcheck/utils/Ether.h>
#include <libsolidity/modelcheck/utils/Function.h>
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

EtherMethodGenerator::EtherMethodGenerator(
    shared_ptr<AnalysisStack const> _stack,
    shared_ptr<NondetSourceRegistry> _nd_reg
): VALUE_T(TypeAnalyzer::get_simple_ctype(
     *CallStateUtilities::get_type(CallStateUtilities::Field::Value)
 ))
 , SENDER_T(TypeAnalyzer::get_simple_ctype(
     *CallStateUtilities::get_type(CallStateUtilities::Field::Sender)
 ))
 , SRC_VAR(make_shared<CVarDecl>(SENDER_T, "src"))
 , BAL_VAR(make_shared<CVarDecl>(VALUE_T, "bal", true))
 , DST_VAR(make_shared<CVarDecl>(SENDER_T, "dst"))
 , AMT_VAR(make_shared<CVarDecl>(VALUE_T, "amt"))
 , m_stack(_stack)
 , m_nd_reg(_nd_reg)
{
}

void EtherMethodGenerator::print(ostream & _stream, bool _forward_declare)
{
    // The pay method is used in isolation.
    if (m_stack->environment()->uses_pay())
    {
        generate_pay(_stream, _forward_declare);
    }

    // The send method is taken as a utility to 
    auto uses_transfer = m_stack->environment()->uses_transfer();
    if (uses_transfer || m_stack->environment()->uses_send())
    {
        generate_send(_stream, _forward_declare);
    }

    if (uses_transfer)
    {
        generate_transfer(_stream, _forward_declare);
    }
}

void EtherMethodGenerator::generate_pay(
    ostream & _stream, bool _forward_declare
)
{
    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto cond = make_shared<CBinaryOp>(
            BAL_VAR->access("v"), ">=", AMT_VAR->access("v")
        );
        auto update = make_shared<CBinaryOp>(
            BAL_VAR->access("v"), "-=", AMT_VAR->access("v")
        );

        string error_msg("Insufficient funds to call.");

        CBlockList statements;
        LibVerify::add_require(statements, cond, error_msg);
        statements.push_back(update->stmt());
        statements.push_back(make_shared<CReturn>(AMT_VAR->id()));
        body = make_shared<CBlock>(move(statements));
    }

    auto id = make_shared<CVarDecl>(VALUE_T, Ether::PAY);
    _stream << CFuncDef(id, CParams{BAL_VAR, AMT_VAR}, body);
}

void EtherMethodGenerator::generate_send(
    ostream & _stream, bool _forward_declare
)
{
    // Generates body.
    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto fail_rv = make_shared<CReturn>(make_shared<CIntLiteral>(0));
        auto nd_result = m_nd_reg->byte("Return value for send/transfer.");
        auto bal_cond = make_shared<CBinaryOp>(
            BAL_VAR->access("v"), "<", AMT_VAR->access("v")
        );
        auto bal_change = make_shared<CBinaryOp>(
            BAL_VAR->access("v"), "-=", AMT_VAR->access("v")
        );

        CBlockList statements;

        // If balance is insufficient this fails.
        statements.push_back(make_shared<CIf>(bal_cond, fail_rv, nullptr));
        statements.push_back(bal_change->stmt());

        // Adds case for the zero address.
        {
            CBlockList handler_list;
            auto addr_cond = make_shared<CBinaryOp>(
                DST_VAR->access("v"), "==", make_shared<CIntLiteral>(0)
            );
            handler_list.push_back(fail_rv);
            auto handler = make_shared<CBlock>(move(handler_list));
            statements.push_back(make_shared<CIf>(addr_cond, handler, nullptr));
        }

        // Adds cases for non-payable contract.
        generate_fallbacks(
            true, fail_rv, statements, m_stack->tight_bundle()->view()
        );

        // If no cases match, we perform a send with some result.
        statements.push_back(make_shared<CReturn>(nd_result));

        body = make_shared<CBlock>(move(statements));
    }

    // Expands parameters.
    CParams params;
    for (auto const& fld: m_stack->environment()->order())
    {
        params.push_back(make_shared<CVarDecl>(fld.type_name, fld.name, false));
    }
    params.push_back(SRC_VAR);
    params.push_back(BAL_VAR);
    params.push_back(DST_VAR);
    params.push_back(AMT_VAR);

    // Generates code.
    auto id = make_shared<CVarDecl>("uint8_t", Ether::SEND);
    _stream << CFuncDef(id, move(params), body);
}

void EtherMethodGenerator::generate_transfer(
    ostream & _stream, bool _forward_declare
)
{
    // Generates body.
    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        string err_msg("Transfer failed.");

        CFuncCallBuilder send_call(Ether::SEND);
        m_stack->environment()->compute_next_state_for(
            send_call, false, true, nullptr
        );
        send_call.push(SRC_VAR->id());
        send_call.push(BAL_VAR->id());
        send_call.push(DST_VAR->id());
        send_call.push(AMT_VAR->id());

        CBlockList statements;
        LibVerify::add_require(statements, send_call.merge_and_pop(), err_msg);
        body = make_shared<CBlock>(move(statements));
    }


    // Expands parameters.
    CParams params;
    for (auto const& fld: m_stack->environment()->order())
    {
        params.push_back(make_shared<CVarDecl>(fld.type_name, fld.name, false));
    }
    params.push_back(SRC_VAR);
    params.push_back(BAL_VAR);
    params.push_back(DST_VAR);
    params.push_back(AMT_VAR);

    // Generates code.
    auto id = make_shared<CVarDecl>("void", Ether::TRANSFER);
    _stream << CFuncDef(id, move(params), body);
}

void EtherMethodGenerator::generate_fallbacks(
    bool _root,
    CStmtPtr const& _error,
    CBlockList & _statements,
    vector<shared_ptr<BundleContract const>> _contracts
) const
{
    for (auto contract : _contracts)
    {
        size_t addr = contract->address();

        auto addr_cond = make_shared<CBinaryOp>(
            DST_VAR->access("v"), "==", make_shared<CIntLiteral>(addr)
        );

        CBlockList handler_list;
        if (contract->details()->is_payable())
        {
            if (contract->can_fallback_through_send())
            {
                string name = "contract_" + to_string(contract->address());
                CExprPtr id = make_shared<CIdentifier>(name, !_root);
                if (_root)
                {
                    id = make_shared<CReference>(id);
                }

                auto fallback = contract->details()->fallback();
                FunctionSpecialization call(*fallback);

                CFuncCallBuilder builder(call.name(0));
                builder.push(id);

                m_stack->environment()->compute_next_state_for(
		            builder,
                    true,
                    true,
                    AMT_VAR->id(),
                    SRC_VAR->id(),
                    BAL_VAR->id()
	            );

                handler_list.push_back(builder.merge_and_pop_stmt());
                LibVerify::log(handler_list, "Fallback at " + name + ".");
            }
            else
            {
                string const& NAME = contract->details()->name();
                string const MSG = "Fallback not allowed in: " + NAME;
                LibVerify::add_assert(handler_list, Literals::ZERO, MSG);
            }
        }
        else
        {
            handler_list.push_back(_error);
        }
        auto handler = make_shared<CBlock>(move(handler_list));
        _statements.push_back(make_shared<CIf>(addr_cond, handler, nullptr));

        generate_fallbacks(false, _error, _statements, contract->children());
    }
}

// -------------------------------------------------------------------------- //

}
}
}
