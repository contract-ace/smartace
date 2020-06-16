/**
 * Generates methods concerning the transfer of Ether.
 * 
 * @date 2020
 */

#pragma once

#include <memory>
#include <ostream>

#include <libsolidity/modelcheck/codegen/Core.h>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class AnalysisStack;

// -------------------------------------------------------------------------- //

/**
 * Generates Ether related methods such as transfer and send. Also generates
 * Ether-related utility methods, such as the pay method for payable contracts.
 */
class EtherMethodGenerator
{
public:
    // Ether usage information is extracted from _stack.
    EtherMethodGenerator(std::shared_ptr<AnalysisStack const> _stack);

    // Prints all methods to _stream. If _forward_Declaration is set, the bodies
    // are elided.
    void print(std::ostream & _stream, bool _forward_declare);

private:
    std::string const VALUE_T;
    std::string const SENDER_T;

    std::shared_ptr<CVarDecl> const BAL_VAR;
    std::shared_ptr<CVarDecl> const DST_VAR;
    std::shared_ptr<CVarDecl> const AMT_VAR;

    std::shared_ptr<AnalysisStack const> m_stack;

    // Helper method to print pay.
    void generate_pay(std::ostream & _stream, bool _forward_declare);

    // Helper method to print send.
    void generate_send(std::ostream & _stream, bool _forward_declare);

    // Helper method to print transfer.
    void generate_transfer(std::ostream & _stream, bool _forward_declare);
};

// -------------------------------------------------------------------------- //

}
}
}
