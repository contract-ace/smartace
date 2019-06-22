/**
 * @date 2019
 * Utility visitor to convert Solidity blocks into verifiable code.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/TypeTranslator.h>
#include <functional>
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

	std::hash<std::string> m_hasher;

    bool visit(IfStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;
    bool visit(Literal const& _node) override;

/*
    virtual bool visit(EnumValue const& _node) { return visitNode(_node); }
    virtual bool visit(VariableDeclaration const& _node) { return visitNode(_node); }
	virtual bool visit(ParameterList const& _node) { return visitNode(_node); }
	virtual bool visit(ModifierInvocation const& _node) { return visitNode(_node); }
    virtual bool visit(Block const& _node) { return visitNode(_node); }
	virtual bool visit(PlaceholderStatement const& _node) { return visitNode(_node); }
virtual bool visit(IfStatement const& _node) { return visitNode(_node); }
	virtual bool visit(WhileStatement const& _node) { return visitNode(_node); }
	virtual bool visit(ForStatement const& _node) { return visitNode(_node); }
	virtual bool visit(Continue const& _node) { return visitNode(_node); }
	virtual bool visit(InlineAssembly const& _node) { return visitNode(_node); }
	virtual bool visit(Break const& _node) { return visitNode(_node); }
	virtual bool visit(Return const& _node) { return visitNode(_node); }
	virtual bool visit(Throw const& _node) { return visitNode(_node); }
    virtual bool visit(VariableDeclarationStatement const& _node) { return visitNode(_node); }
virtual bool visit(ExpressionStatement const& _node) { return visitNode(_node); }
	virtual bool visit(Conditional const& _node) { return visitNode(_node); }
	virtual bool visit(Assignment const& _node) { return visitNode(_node); }
	virtual bool visit(TupleExpression const& _node) { return visitNode(_node); }
	virtual bool visit(UnaryOperation const& _node) { return visitNode(_node); }
	virtual bool visit(BinaryOperation const& _node) { return visitNode(_node); }
	virtual bool visit(FunctionCall const& _node) { return visitNode(_node); }
	virtual bool visit(NewExpression const& _node) { return visitNode(_node); }
	virtual bool visit(MemberAccess const& _node) { return visitNode(_node); }
	virtual bool visit(IndexAccess const& _node) { return visitNode(_node); }
	virtual bool visit(Identifier const& _node) { return visitNode(_node); }
	virtual bool visit(ElementaryTypeNameExpression const& _node) { return visitNode(_node); }
virtual bool visit(Literal const& _node) { return visitNode(_node); }
*/
};

}
}
}
