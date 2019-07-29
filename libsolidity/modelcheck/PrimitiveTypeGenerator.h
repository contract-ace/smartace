/**
 * @date 2019
 * Visitor used to generate all primitive type declarations. This will produce
 * a header-only library for arithmetic operations.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/SimpleCCore.h>
#include <array>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

/**
 * The class provides an analysis tool, which will determine all primitve types
 * in use, and then produce the primitive type declarations required to
 * translate the given AST into C. 
 */
class PrimitiveTypeGenerator
{
public:
    // Analyzes primitive type usage in _root, in order to generate datatypes.
    PrimitiveTypeGenerator(ASTNode const& _root);

    // Accesses to see if a type was found.
    bool found_bool();
    bool found_address();
    bool found_int(unsigned char _bytes);
    bool found_uint(unsigned char _bytes);
    bool found_fixed(unsigned char _bytes, unsigned char _d);
    bool found_ufixed(unsigned char _bytes, unsigned char _d);

private:
    // Auxliary class which can visit the AST once, and then cache all results.
    class Visitor : public ASTConstVisitor 
    {
    public:
        // Caches primitive type data from the declarations in _root.
        Visitor(ASTNode const& _root);

        bool m_uses_address = false;
        bool m_uses_bool = false;

        std::array<bool, 32> m_uses_int;
        std::array<bool, 32> m_uses_uint;
        std::array<std::array<bool, 81>, 32> m_uses_fixed;
        std::array<std::array<bool, 81>, 32> m_uses_ufixed;

    protected:
        void endVisit(UsingForDirective const& _node) override;
        void endVisit(VariableDeclaration const& _node) override;
        void endVisit(ElementaryTypeName const& _node) override;
        void endVisit(ElementaryTypeNameExpression const& _node) override;
    
    private:
        // Records the usage of _type in the AST.
        void process_type(Type const* _type);
    };

    Visitor const M_SUMMARY;
};

}
}
}
