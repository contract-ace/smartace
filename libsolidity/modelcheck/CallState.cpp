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

void CallState::record(ASTNode const& _ast)
{
    _ast.accept(*this);
}

// -------------------------------------------------------------------------- //

void CallState::print(std::ostream& _stream, bool _forward_declare) const
{
    shared_ptr<CParams> cs_fields;
    shared_ptr<CBlock> pay_body;

    auto addr_type = TypeConverter::get_simple_ctype(
        AddressType(StateMutability::Payable)
    );
    auto uint256_type = TypeConverter::get_simple_ctype(IntegerType(256));

    // TODO(scottwe): Required fields should be discovered.
    if (!_forward_declare)
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

    _stream << cs << pay;
}

// -------------------------------------------------------------------------- //

void CallState::register_primitives(PrimitiveTypeGenerator& _gen) const
{
    // TODO(scottwe): See below; this should not be hard-coded...
    _gen.record_address();
    _gen.record_uint(256);
}

// -------------------------------------------------------------------------- //

}
}
}
