/**
 * Expands inheritance and allocation data into a tree of tightly coupled
 * contracts. This tree gives a unique identifier to each contract instance.
 * 
 * @date 2020
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallState;
class FlatContract;
class FlatModel;

// -------------------------------------------------------------------------- //

/**
 * A node in a tree of contract instances. The tree represents the ownership of
 * instances. This tree is the canonical assignment of contract addresses.
 */
class BundleContract
{
public:
    // Generates a node for _contract in _model, starting with address _id and
    // using field _var.
    BundleContract(
        FlatModel const& _model,
        CallState const& _env,
        bool _global_fallbacks,
        std::shared_ptr<FlatContract> _contract,
        uint64_t & _id,
        std::string const& _var
    );

    // Returns the unique address of this contract instance.
    uint64_t address() const;

    // Returns the field name used to access this instance.
    std::string const& var() const;

    // Returns the underlying flat contract.
    std::shared_ptr<FlatContract> details() const;

    // Returns true if a call to send can result in a fallback call.
    bool can_fallback_through_send() const;

    // Return the contract instance for each child.
    std::vector<std::shared_ptr<BundleContract const>> const& children() const;

private:
    uint64_t m_id;
    bool m_fallback = false;
    std::string m_var;
    std::shared_ptr<FlatContract> m_contact;
    std::vector<std::shared_ptr<BundleContract const>> m_children;
};

// -------------------------------------------------------------------------- //

/**
 * An interface to contracts that captures the ownership structure, and resolves
 * ownership issues such as variable names and addresses.
 */
class TightBundleModel
{
public:
    // Expands all allocations in _model. If _global_fallbacks is true, and
    // _env allows for fallsbacks through send, then contracts with fallback
    // functions will be marked by `can_fallback_through_send()`.
    TightBundleModel(
        FlatModel const& _model, CallState const& _env, bool _global_fallbacks
    );

    // Returns the number of allocated contracts.
    uint64_t size() const;

    // Returns the top level allocations in the bundle.
    std::vector<std::shared_ptr<BundleContract const>> const& view() const;

private:
    uint64_t m_size = 0;
    std::vector<std::shared_ptr<BundleContract const>> m_top_level;
};

// -------------------------------------------------------------------------- //

}
}
}
