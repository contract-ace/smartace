#include <libsolidity/modelcheck/scheduler/StateGenerator.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/utils/CallState.h>

#include <memory>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

const string StateGenerator::LAST_SENDER = "last_sender";

// -------------------------------------------------------------------------- //

StateGenerator::StateGenerator(
    shared_ptr<AnalysisStack const> _stack,
    shared_ptr<NondetSourceRegistry> _nd_reg,
    bool _use_lockstep_time
): M_USE_LOCKSTEP_TIME(_use_lockstep_time), m_stack(_stack), m_nd_reg(_nd_reg)
{
}

// -------------------------------------------------------------------------- //

void StateGenerator::declare(CBlockList & _block) const
{
    for (auto const& fld : m_stack->environment()->order())
    {
        // Determines the initial value, given it should be made global at all.
        CExprPtr val;
        if (fld.field == CallStateUtilities::Field::Block ||
            fld.field == CallStateUtilities::Field::Timestamp)
        {
            val = Literals::ZERO;
            if (M_USE_LOCKSTEP_TIME)
            {
                val = m_nd_reg->raw_val(*fld.type, fld.name);
            }
        }
        else if (fld.field == CallStateUtilities::Field::Paid)
        {
            val = Literals::ONE;
        }

        // Initializes the ID.
        if (val)
        {
            auto id = make_shared<CVarDecl>(fld.type_name, fld.name);
            _block.push_back(id);
            _block.push_back(id->access("v")->assign(val)->stmt());
        }
        else if (fld.field == CallStateUtilities::Field::Sender)
        {
            auto id = make_shared<CVarDecl>(fld.type_name, LAST_SENDER);
            _block.push_back(id);
        }
    }
}

// -------------------------------------------------------------------------- //

void StateGenerator::update_global(CBlockList & _block) const
{
    CBlockList step_block_list;

    // Constructs step block.
    for (auto const& fld : m_stack->environment()->order())
    {
        if (fld.field == CallStateUtilities::Field::Block ||
            fld.field == CallStateUtilities::Field::Timestamp)
        {
            auto state = make_shared<CIdentifier>(fld.name, false);
            auto step = state->access("v")->assign(m_nd_reg->increase(
                state->access("v"), M_USE_LOCKSTEP_TIME, fld.name
            ))->stmt();
            step_block_list.push_back(step);
        }
    }

    // Performs step.
    auto step_block = make_shared<CBlock>(move(step_block_list));
    if (M_USE_LOCKSTEP_TIME)
    {
        auto choice = m_nd_reg->range(0, 2, "take_step");
        _block.push_back(make_shared<CIf>(choice, step_block));
    }
    else
    {
        _block.push_back(step_block);
    }
}

// -------------------------------------------------------------------------- //

void StateGenerator::update_local(CBlockList & _block) const
{
    for (auto const& fld : m_stack->environment()->order())
    {
        if (fld.field == CallStateUtilities::Field::Paid) continue;
        if (fld.field == CallStateUtilities::Field::Origin) continue;
        if (fld.field == CallStateUtilities::Field::Block) continue;
        if (fld.field == CallStateUtilities::Field::Timestamp) continue;

        // Generates txn field.
        CExprPtr val;
        if (fld.field == CallStateUtilities::Field::Value)
        {
            val = Literals::ZERO;
        }
        else if (fld.field == CallStateUtilities::Field::Sender)
        {
            // This restricts senders to valid addresses: non-zero clients.
            size_t minaddr = m_stack->addresses()->contract_count();
            size_t maxaddr = m_stack->addresses()->max_sender();
            if (m_stack->addresses()->literals().find(0)
                != m_stack->addresses()->literals().end())
            {
                minaddr += 1;
            }
            val = m_nd_reg->range(minaddr, maxaddr, fld.name);
        }
        else if (fld.field == CallStateUtilities::Field::ReqFail)
        {
            val = Literals::ONE;
        }
        else
        {
            val = m_nd_reg->raw_val(*fld.type, fld.name);
        }

        // Generates source code.
        auto decl = make_shared<CVarDecl>(fld.type_name, fld.name);
        _block.push_back(decl);
        _block.push_back(decl->access("v")->assign(val)->stmt());

        // Stores txn selection for use in properties.
        if (fld.field == CallStateUtilities::Field::Sender)
        {
            auto last = make_shared<CIdentifier>(LAST_SENDER, false);
            auto curr = decl->id()->access("v");
            _block.push_back(last->access("v")->assign(curr)->stmt());
        }
    }
}

// -------------------------------------------------------------------------- //

void StateGenerator::pay(CBlockList & _block) const
{
    auto const VAL_FIELD = CallStateUtilities::Field::Value;
    auto const VAL_NAME = CallStateUtilities::get_name(VAL_FIELD);
    auto const VAL_TYPE = CallStateUtilities::get_type(VAL_FIELD);

    auto nd = m_nd_reg->raw_val(*VAL_TYPE, VAL_NAME);
    auto state = make_shared<CIdentifier>(VAL_NAME, false);

    _block.push_back(state->access("v")->assign(nd)->stmt());
}

// -------------------------------------------------------------------------- //

}
}
}
