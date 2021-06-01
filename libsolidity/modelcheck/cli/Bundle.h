/**
 * Utilities to extract a bundle from a smart contract program.
 * 
 * @date 2021
 */

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{

class ContractDefinition;
class SourceUnit;

namespace modelcheck
{

// -------------------------------------------------------------------------- //

class BundleExtractor
{
public:
    // Extracts contracts from _asts, with names appearing in _names.
    BundleExtractor(
        std::vector<SourceUnit const*> _asts, std::vector<std::string> _names
    );

    // Returns the model.
    std::vector<ContractDefinition const*> const& get() const;

    // Returns a list of missing contract (names).
    std::vector<std::string> const& missing() const;

private:
    // The list of extracted contracts.
    std::vector<ContractDefinition const*> m_contracts;

    // The list of missing contracts.
    std::vector<std::string> m_missing;
};


// -------------------------------------------------------------------------- //

}
}
}
