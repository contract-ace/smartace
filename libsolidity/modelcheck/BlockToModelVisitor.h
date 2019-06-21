/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <ostream>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * A utility visitor, designed to convert Solidity code blocks into executable
 * C code. This is meant to be used a utility when converting a full Solidity
 * source unit. This splits data structure conversion from instruction
 * conversion.
 */
class BlockToModelVisitor : public ASTConstVisitor
{
public:
    // Creates a C model code generator for a given block of Solidity code.
    BlockToModelVisitor(
        Block const& _body,
        TypeTranslator const& _scope
    );

    // Generates a human-readable block of C code, from the given block.
    void print(std::ostream& _stream);

protected:
    Block const* m_body;
    TypeTranslator const& m_scope;
	std::ostream* m_ostream = nullptr;

    bool visit(IfStatement const& _node) override;
};

}
}
}
