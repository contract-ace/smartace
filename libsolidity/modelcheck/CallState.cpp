/**
 * @date 2019
 * First-pass visitor for generating the CallState of Solidity in C models,
 * which consist of the struct of CallState.
 */

#include <libsolidity/modelcheck/CallState.h>

#include <libsolidity/modelcheck/PrimitiveTypeGenerator.h>
#include <libsolidity/modelcheck/SimpleCGenerator.h>
#include <libsolidity/modelcheck/Utility.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CallState::CallState(
    ASTNode const& _ast, bool _forward_declare
): m_ast(_ast), m_forward_declare(_forward_declare)
{
}

// -------------------------------------------------------------------------- //

void CallState::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}

// -------------------------------------------------------------------------- //

void CallState::register_primitives(PrimitiveTypeGenerator& _gen)
{
    // TODO(scottwe): See below; this should not be hard-coded...
    _gen.record_address();
    _gen.record_uint(256);
}

// -------------------------------------------------------------------------- //

void CallState::endVisit(ContractDefinition const& _node)
{
    (void) _node;

    shared_ptr<CParams> cs_fields;
    shared_ptr<CBlock> pay_body;

    auto addr_type = TypeConverter::get_simple_ctype(
        AddressType(StateMutability::Payable)
    );
    auto uint256_type = TypeConverter::get_simple_ctype(IntegerType(256));

    // TODO(scottwe): Required fields should be discovered.
    if (!m_forward_declare)
    {
        cs_fields = make_shared<CParams>();
        cs_fields->push_back(make_shared<CVarDecl>(addr_type, "sender"));
        cs_fields->push_back(make_shared<CVarDecl>(uint256_type, "value"));
        cs_fields->push_back(make_shared<CVarDecl>(uint256_type, "blocknum"));

        pay_body = make_shared<CBlock>(CBlockList{});
    }

    CStructDef cs("CallState", move(cs_fields));
    CFuncDef pay(
        make_shared<CVarDecl>("void", "_pay"), CParams{
            cs.decl("state", true),
            make_shared<CVarDecl>(addr_type, "dst"),
            make_shared<CVarDecl>(uint256_type, "amt")
        }, move(pay_body)
    );

    (*m_ostream) << cs << pay;
}

// -------------------------------------------------------------------------- //

}
}
}
