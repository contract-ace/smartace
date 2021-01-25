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

class FlatContract;
class FlatModel;

// -------------------------------------------------------------------------- //

/**
 * 
 */
class BundleContract
{
public:
    // 
    BundleContract(
        FlatModel const& _model,
        std::shared_ptr<FlatContract> _contract,
        uint64_t & _id,
        std::string const& _var
    );

    // 
    uint64_t address() const;

    //
    std::string const& var() const;

    //
    std::shared_ptr<FlatContract> details() const;

    //
    std::vector<std::shared_ptr<BundleContract const>> const& children() const;

private:
    uint64_t m_id;
    std::string m_var;
    std::shared_ptr<FlatContract> m_contact;
    std::vector<std::shared_ptr<BundleContract const>> m_children;
};

// -------------------------------------------------------------------------- //

/**
 * 
 */
class TightBundleModel
{
public:
    // Expands all allocations in _model.
    TightBundleModel(FlatModel const& _model);

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
