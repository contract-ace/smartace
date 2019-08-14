/**
 * @date 2019
 * Visitor used to generate all primitive type declarations. This will produce
 * a header-only library for arithmetic operations.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/SimpleCCore.h>
#include <array>
#include <map>

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
class PrimitiveTypeGenerator: protected ASTConstVisitor
{
public:
    // Analyzes primitive type usage in _root, in order to generate datatypes.
    PrimitiveTypeGenerator();

    // Registers all primitive types from the given file.
    void record(ASTNode const& _root);

    // Accesses to see if a type was found.
    bool found_bool() const;
    bool found_address() const;
    bool found_int(unsigned char _bytes) const;
    bool found_uint(unsigned char _bytes) const;
    bool found_fixed(unsigned char _bytes, unsigned char _d) const;
    bool found_ufixed(unsigned char _bytes, unsigned char _d) const;

    // Generates the primitive type definitions, as required by AST.
    void print(std::ostream& _out) const;

protected:
    void endVisit(UsingForDirective const& _node) override;
    void endVisit(VariableDeclaration const& _node) override;
    void endVisit(ElementaryTypeName const& _node) override;
    void endVisit(ElementaryTypeNameExpression const& _node) override;
    void endVisit(FunctionCall const& _node) override;

private:
    // Auxilary class which generates data needed for Integers and FixedPoint.
    class EncodingData
    {
    public:
        // Generates data needed to encode the given _bytes, and _signed value.
        EncodingData(unsigned char _bytes, bool _signed);

        unsigned short bits;
        bool is_native_width;
        bool is_aligned_width;
        std::string base;
    };

    // Starting from int/uint/fixed/ufixed, these methods abstract out common
    // formatting behaviour shared between these data-types, or subsets of these
    // data types (ie int/uint -> integer, or _bytes < 64 -> native).
    static void declare_integer(
        std::ostream& _out, unsigned char _bytes, bool _signed
    );
    static void declare_fixed(
        std::ostream& _out, unsigned char _bytes, unsigned char _pt, bool _signed
    );
    static void declare_padded_native(
        std::ostream& _out, std::string const& _sym, EncodingData const& _data
    );

    // TODO
    static void print_initializer(
        std::ostream& _out, std::string const& _type, std::string const& _data
    );

    // Records the usage of _type in the AST.
    void process_type(Type const* _type);

    bool m_uses_address = false;
    bool m_uses_bool = false;

    std::array<bool, 32> m_uses_int;
    std::array<bool, 32> m_uses_uint;
    std::array<std::array<bool, 81>, 32> m_uses_fixed;
    std::array<std::array<bool, 81>, 32> m_uses_ufixed;
};

}
}
}
