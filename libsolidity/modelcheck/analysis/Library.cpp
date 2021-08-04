#include <libsolidity/modelcheck/analysis/Library.h>

#include<libsolidity/modelcheck/analysis/CallGraph.h>

#include <map>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallGraph;

// -------------------------------------------------------------------------- //

Library::Library(
    ContractDefinition const& _library,
    vector<FunctionDefinition const*> _calls,
    shared_ptr<StructureStore> _store
): StructureContainer(_library, _store), m_functions(_calls)
{
}

vector<FunctionDefinition const*> Library::functions() const
{
    return m_functions;
}

// -------------------------------------------------------------------------- //

LibrarySummary::LibrarySummary(
    CallGraph const& _calls, shared_ptr<StructureStore> _store
) {
    map<ContractDefinition const*, vector<FunctionDefinition const*>> libraries;

    // Computes libraries with structures in use.
    for (auto itr = _store->begin(); itr != _store->end(); ++itr)
    {
        auto scope = itr->first->scope();
        if (auto contract = dynamic_cast<ContractDefinition const*>(scope))
        {
            if (contract->isLibrary())
            {
                if (libraries.count(contract) == 0)
                {
                    libraries[contract] = {};
                }
            }
        }
    }

    // Computes library usage across all contract methods.
    for (auto func : _calls.executed_code())
    {
        auto contract = dynamic_cast<ContractDefinition const*>(func->scope());
        if (contract->isLibrary())
        {
            libraries[contract].push_back(func);
        }
    }

    // Generates the mappings.
    m_libraries.reserve(libraries.size());
    for (auto lib : libraries)
    {
        auto record = make_shared<Library>(*lib.first, lib.second, _store);
        m_libraries.push_back(std::move(record));
    }
}

vector<shared_ptr<Library const>> LibrarySummary::view() const
{
    return m_libraries;
}

// -------------------------------------------------------------------------- //

}
}
}
