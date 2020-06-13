#include <libsolidity/modelcheck/scheduler/StateGenerator.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/utils/CallState.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>

#include <memory>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

StateGenerator::StateGenerator(
    shared_ptr<AnalysisStack const> _stack, bool _use_lockstep_time
): M_USE_LOCKSTEP_TIME(_use_lockstep_time)
 , M_STEPVAR(make_shared<CVarDecl>("uint8_t", "take_step"))
 , m_stack(_stack)
{
}

// -------------------------------------------------------------------------- //

void StateGenerator::declare(CBlockList & _block) const
{
    if (M_USE_LOCKSTEP_TIME)
    {
        _block.push_back(M_STEPVAR);
    }
    for (auto const& fld : m_stack->environment()->order())
    {
        auto const DECL = make_shared<CVarDecl>(fld.type_name, fld.name);
        _block.push_back(DECL);

        if (fld.field == CallStateUtilities::Field::Block ||
            fld.field == CallStateUtilities::Field::Timestamp)
        {
            if (M_USE_LOCKSTEP_TIME)
            {
                auto nd = m_stack->types()->raw_simple_nd(*fld.type, fld.name);
                _block.push_back(
                    DECL->access("v")->assign(nd)->stmt()
                );
            }
            else
            {
                _block.push_back(
                    DECL->access("v")->assign(Literals::ZERO)->stmt()
                );
            }
        }
        else if (fld.field == CallStateUtilities::Field::Paid)
        {
            _block.push_back(DECL->access("v")->assign(Literals::ONE)->stmt());
        }
    }
}

// -------------------------------------------------------------------------- //

void StateGenerator::update(CBlockList & _block) const
{
    // Decides once, if lockstep will be used.
    if (M_USE_LOCKSTEP_TIME)
    {
        _block.push_back(M_STEPVAR->id()->assign(
            LibVerify::range(0, 2, "take_step"))->stmt()
        );
    }

    // Updates the values.
    for (auto const& fld : m_stack->environment()->order())
    {
        auto state = make_shared<CIdentifier>(fld.name, false);
        auto nd = m_stack->types()->raw_simple_nd(*fld.type, fld.name);

        if (fld.field == CallStateUtilities::Field::Paid) continue;
        if (fld.field == CallStateUtilities::Field::Origin) continue;

        if (fld.field == CallStateUtilities::Field::Block ||
            fld.field == CallStateUtilities::Field::Timestamp)
        {
            auto step = state->access("v")->assign(LibVerify::increase(
                state->access("v"), M_USE_LOCKSTEP_TIME, fld.name
            ))->stmt();
            if (M_USE_LOCKSTEP_TIME)
            {
                _block.push_back(make_shared<CIf>(
                    M_STEPVAR->id(),
                    make_shared<CBlock>(CBlockList{ move(step) })
                ));
            }
            else
            {
                _block.push_back(step);
            }
        }
        else if (fld.field == CallStateUtilities::Field::Value)
        {
            _block.push_back(state->access("v")->assign(Literals::ZERO)->stmt());
        }
        else if (fld.field == CallStateUtilities::Field::Sender)
        {
            // This restricts senders to valid addresses: non-zero clients.
            size_t minaddr = m_stack->addresses()->contract_count();
            size_t maxaddr = m_stack->addresses()->size();
            if (m_stack->addresses()->literals().find(0)
                != m_stack->addresses()->literals().end())
            {
                minaddr += 1;
            }

            auto nd_addr = LibVerify::range(minaddr, maxaddr, fld.name);
            _block.push_back(state->access("v")->assign(nd_addr)->stmt());
        }
        else
        {
            _block.push_back(state->access("v")->assign(nd)->stmt());
        }
    }
}

// -------------------------------------------------------------------------- //

void StateGenerator::pay(CBlockList & _block) const
{
    auto const VAL_FIELD = CallStateUtilities::Field::Value;
    auto const VAL_NAME = CallStateUtilities::get_name(VAL_FIELD);
    auto const VAL_TYPE = CallStateUtilities::get_type(VAL_FIELD);

    auto nd = m_stack->types()->raw_simple_nd(*VAL_TYPE, VAL_NAME);
    auto state = make_shared<CIdentifier>(VAL_NAME, false);

    _block.push_back(state->access("v")->assign(nd)->stmt());
}

// -------------------------------------------------------------------------- //

}
}
}
