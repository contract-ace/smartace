#include <libsolidity/modelcheck/analysis/TightBundle.h>

#include <libsolidity/modelcheck/analysis/CallState.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

BundleContract::BundleContract(
    FlatModel const& _model,
    CallState const& _env,
    bool _global_fallbacks,
    shared_ptr<FlatContract> _contract,
    uint64_t & _id,
    string const& _var
): m_id(_id), m_var(_var), m_contact(_contract)
{
    // Updates address counter before visiting children.
    ++_id;

    // Sets fallback flag if contract has a fallback and send is used.
    if (_global_fallbacks && _contract->fallback())
    {
        m_fallback = (_env.uses_send() || _env.uses_transfer());
    }

    // Expands each child.
    for (auto entry : _model.children_of(*_contract))
    {
        m_children.push_back(make_shared<BundleContract>(
            _model, _env, _global_fallbacks, entry.child, _id, entry.var
        ));
    }
}

uint64_t BundleContract::address() const
{
    // Accounts for the zero address.
    return m_id + 1;
}

string const& BundleContract::var() const
{
    return m_var;
}

shared_ptr<FlatContract> BundleContract::details() const
{
    return m_contact;
}

bool BundleContract::can_fallback_through_send() const
{
    return m_fallback;
}

vector<std::shared_ptr<BundleContract const>> const&
    BundleContract::children() const
{
    return m_children;
}

// -------------------------------------------------------------------------- //

TightBundleModel::TightBundleModel(
    FlatModel const& _model, CallState const& _env, bool _global_fallbacks
) {
    for (auto contract : _model.bundle())
    {
        m_top_level.push_back(make_shared<BundleContract>(
            _model, _env, _global_fallbacks, contract, m_size, ""
        ));
    }
}

uint64_t TightBundleModel::size() const
{
    return m_size;
}

vector<shared_ptr<BundleContract const>> const& TightBundleModel::view() const
{
    return m_top_level;
}

// -------------------------------------------------------------------------- //

}
}
}