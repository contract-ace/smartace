#include <libsolidity/modelcheck/cli/Bundle.h>

using namespace std;

#include <map>

#include <libsolidity/ast/AST.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

BundleExtractor::BundleExtractor(
    vector<SourceUnit const*> _asts, vector<string> _names
)
{
	// Maps names to contracts.
	map<string, ContractDefinition const*> contract_names;
    for (auto const* ast : _asts)
    {
        auto actors = ASTNode::filteredNodes<ContractDefinition>(ast->nodes());
        for (auto actor : actors)
        {
            if (actor->isLibrary()) continue;
            if (actor->isInterface()) continue;
            contract_names[actor->name()] = actor;
        }
    }

    // Resolves bundle names.
    m_contracts.reserve(_names.size());
    for (auto name : _names)
    {
        if (contract_names[name] == nullptr)
        {
            m_missing.push_back(name);
        }
        else
        {
            m_contracts.push_back(contract_names[name]);
        }
    }
}

vector<ContractDefinition const*> const& BundleExtractor::get() const
{
    return m_contracts;
}

vector<string> const& BundleExtractor::missing() const
{
    return m_missing;
}

// -------------------------------------------------------------------------- //

}
}
}
