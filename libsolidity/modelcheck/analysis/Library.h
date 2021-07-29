/**
 * Analysis tools to identify and restrict libraries.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/modelcheck/analysis/Structure.h>

#include <memory>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class CallGraph;

// -------------------------------------------------------------------------- //

/**
 * A restricted mapping interface designed to more readily provide needed
 * information in analysis.
 */
class Library : public StructureContainer
{
public:
    // Generates a wrapper to _library, restricted to the methods in _calls
    Library(
        ContractDefinition const& _library,
        std::vector<FunctionDefinition const*> _calls,
        std::shared_ptr<StructureStore> _store
    );

    // Returns the used methods of the library.
    std::vector<FunctionDefinition const*> functions() const;

private:
    std::vector<FunctionDefinition const*> m_functions;
};

/**
 * Summarizes all libraries used within a call graph.
 */
class LibrarySummary
{
public:
    // Summarizes all libraries used in _calls.
    LibrarySummary(
        CallGraph const& _calls, std::shared_ptr<StructureStore> _store
    );

    // Gives view of all accessible libraries.
    std::vector<std::shared_ptr<Library const>> view() const;

private:
    std::vector<std::shared_ptr<Library const>> m_libraries;
};

// -------------------------------------------------------------------------- //

}
}
}
