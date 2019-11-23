/**
 * @date 2019
 * Visitor used to generate all primitive type declarations. This will produce
 * a header-only library for arithmetic operations.
 */

#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/modelcheck/codegen/Core.h>
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

    // Records a single type.
    void record_type(Type const* _type);

    // Public interface to directly register types.
    void record_bool();
    void record_address();
    void record_int(uint16_t _bits);
    void record_uint(uint16_t _bits);
    void record_fixed(uint16_t _bits, uint16_t _pt);
    void record_ufixed(uint16_t _bits, uint16_t _pt);

    // Accesses to see if a type was found.
    bool found_bool() const;
    bool found_address() const;
    bool found_int(uint8_t _bytes) const;
    bool found_uint(uint8_t _bytes) const;
    bool found_fixed(uint8_t _bytes, uint8_t _d) const;
    bool found_ufixed(uint8_t _bytes, uint8_t _d) const;

    // Generates the primitive type definitions, as required by AST.
    void print(std::ostream& _out) const;

protected:
    void endVisit(Mapping const&) override;
    void endVisit(UsingForDirective const& _node) override;
    void endVisit(VariableDeclaration const& _node) override;
    void endVisit(ElementaryTypeName const& _node) override;
    void endVisit(ElementaryTypeNameExpression const& _node) override;
    void endVisit(FunctionCall const& _node) override;

private:
    // There is common formatting behaviours between (int/uint), (fixed/ufixed),
    // all of (int/uint/fixed/ufixed) and the set of all primitives. These
    // methods factor out those behaviours to minimize duplication.
    static void declare_integer(
        std::ostream& _out, uint8_t _bytes, bool _signed
    );
    static void declare_fixed(
        std::ostream& _out, uint8_t _bytes, uint8_t _pt, bool _signed
    );
    static void declare_numeric(
        std::ostream& _out, std::string const& _sym, uint8_t _bytes, bool _signed
    );
    static void declare_primitive(
        std::ostream& _out, std::string const& _type, std::string const& _data
    );

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
