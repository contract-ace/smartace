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
    ContractDefinition const& _library, list<FunctionDefinition const*> _calls
): StructureContainer(_library), m_functions(_calls)
{
}

list<FunctionDefinition const*> Library::functions() const
{
    return m_functions;
}

// -------------------------------------------------------------------------- //

LibrarySummary::LibrarySummary(CallGraph const& _calls)
{
    // Computes library usage across all code.
    map<ContractDefinition const*, list<FunctionDefinition const*>> libraries;
    for (auto func : _calls.executed_code())
    {
        auto contract = dynamic_cast<ContractDefinition const*>(func->scope());
        if (contract->isLibrary())
        {
            libraries[contract].push_back(func);
        }
    }

    // Generates the mappings.
    for (auto lib : libraries)
    {
        m_libraries.push_back(make_shared<Library>(*lib.first, lib.second));
    }
}

list<shared_ptr<Library const>> LibrarySummary::view() const
{
    return m_libraries;
}

// -------------------------------------------------------------------------- //

}
}
}
