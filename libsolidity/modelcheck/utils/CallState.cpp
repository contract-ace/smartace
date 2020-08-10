#include <libsolidity/modelcheck/utils/CallState.h>

#include <stdexcept>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

map<pair<MagicType::Kind, string>, CallStateUtilities::Field> const 
    CallStateUtilities::MAGIC_TYPE_LOOKUP{{
	{{MagicType::Kind::Block, "number"}, Field::Block},
	{{MagicType::Kind::Block, "timestamp"}, Field::Timestamp},
	{{MagicType::Kind::Message, "sender"}, Field::Sender},
	{{MagicType::Kind::Message, "value"}, Field::Value},
    {{MagicType::Kind::Transaction, "origin"}, Field::Origin}
}};

AddressType const CallStateUtilities::SENDER_TYPE(StateMutability::Payable);

IntegerType const CallStateUtilities::COUNTABLE_TYPE(256);

BoolType const CallStateUtilities::BOOLEAN_TYPE;

// -------------------------------------------------------------------------- //

CallStateUtilities::Field CallStateUtilities::parse_magic_type(
    Type const& _type, string _field
)
{
	auto const MAGIC_TYPE = dynamic_cast<MagicType const*>(&_type);
	if (!MAGIC_TYPE)
	{
		throw runtime_error("Resolution of MagicType failed in MemberAccess.");
	}

	auto const RES = MAGIC_TYPE_LOOKUP.find({MAGIC_TYPE->kind(), _field});
	if (RES == MAGIC_TYPE_LOOKUP.end())
	{
		throw runtime_error("Unable to resolve member of Magic type.");
	}
	return RES->second;
}

// -------------------------------------------------------------------------- //

string CallStateUtilities::get_name(CallStateUtilities::Field _field)
{
    switch (_field)
    {
    case CallStateUtilities::Field::Sender: return "sender";
    case CallStateUtilities::Field::Value: return "value";
    case CallStateUtilities::Field::Block: return "blocknum";
    case CallStateUtilities::Field::Timestamp: return "timestamp";
    case CallStateUtilities::Field::Paid: return "paid";
    case CallStateUtilities::Field::Origin: return "origin";
    case CallStateUtilities::Field::ReqFail: return "reqfail";
    default: return "unknowntype";
    };
}

// -------------------------------------------------------------------------- //

Type const* CallStateUtilities::get_type(CallStateUtilities::Field _field)
{
    switch (_field)
    {
    case CallStateUtilities::Field::Sender: return &SENDER_TYPE;
    case CallStateUtilities::Field::Value: return &COUNTABLE_TYPE;
    case CallStateUtilities::Field::Block: return &COUNTABLE_TYPE;
    case CallStateUtilities::Field::Timestamp: return &COUNTABLE_TYPE;
    case CallStateUtilities::Field::Paid: return &BOOLEAN_TYPE;
    case CallStateUtilities::Field::Origin: return &SENDER_TYPE;
    case CallStateUtilities::Field::ReqFail: return &BOOLEAN_TYPE;
    default: return nullptr;
    };
}

// -------------------------------------------------------------------------- //

bool CallStateUtilities::is_contract_only(CallStateUtilities::Field _field)
{
    switch (_field)
    {
    case CallStateUtilities::Field::ReqFail: return false;
    default: return true;
    };
}

// -------------------------------------------------------------------------- //

}
}
}
