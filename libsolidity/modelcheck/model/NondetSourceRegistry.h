/**
 * Generates unique non-deterministic sources for each non-deterministic call.
 * 
 * @date 2020
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/analysis/AnalysisStack.h>
#include <libsolidity/modelcheck/codegen/Core.h>

#include <memory>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

/**
 * Shared by other model generates to achieve unique indices. This is done in
 * place of a global counter.
 */
class NondetSourceRegistry
{
public:
    // The non-deterministic registry will use _stack to determin the address
    // count. This is used for non-deterministic address values.
    NondetSourceRegistry(std::shared_ptr<AnalysisStack const> _stack);

    // Requests a non-deterministic byte, as described by _msg.
    CExprPtr byte(std::string _msg);

    // Requests a non-deterministic byte from [_l, _u), as described by _msg.
    CExprPtr range(uint8_t _l, uint8_t _u, std::string const& _msg);

    // Requests a value at least as large as _curr. If _strict is set, the value
    // should be strictly increasing. The _msg describes the update.
    CExprPtr increase(CExprPtr _curr, bool _strict, std::string _msg);

    // Requests a non-deterministic value for primitive _type described by _msg.
    CExprPtr raw_val(Type const& _type, std::string const& _msg);

    // Requests a non-deterministic value for _type described by _msg.
    CExprPtr simple_val(Type const& _type, std::string const& _msg);

    // Requests a non-deterministic value for _type described by _msg.
	CExprPtr val(TypeName const& _type, std::string const& _msg);

    // Requests a non-deterministic value for _decl described by _msg.
	CExprPtr val(Declaration const& _decl, std::string const& _msg);

    // Prints all non-deterministic methods to _stream.
    void print(std::ostream& _stream);

private:
    std::vector<Type const*> m_registry;

    std::shared_ptr<AnalysisStack const> m_stack;

    // Adds a record for _type into m_registry.
    size_t record(Type const& _type);
};

// -------------------------------------------------------------------------- //

}
}
}
