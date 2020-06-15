/**
 * Analysis tools to identify and restrict libraries.
 * 
 * @date 2019
 */

#pragma once

#include <libsolidity/modelcheck/analysis/Structure.h>

#include <list>
#include <memory>

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
        std::list<FunctionDefinition const*> _calls
    );

    // Returns the used methods of the library.
    std::list<FunctionDefinition const*> functions() const;

private:
    std::list<FunctionDefinition const*> m_functions;
};

/**
 * Summarizes all libraries used within a call graph.
 */
class LibrarySummary
{
public:
    // Summarizes all libraries used in _calls.
    LibrarySummary(CallGraph const& _calls);

    //
    std::list<std::shared_ptr<Library const>> view() const;

private:
    std::list<std::shared_ptr<Library const>> m_libraries;
};

// -------------------------------------------------------------------------- //

}
}
}
