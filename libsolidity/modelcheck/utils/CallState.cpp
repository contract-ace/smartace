/**
 * @date 2019
 * Data and helper functions for generating CallState arguments.
 */

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
	{{MagicType::Kind::Block, "number"}, CallStateUtilities::Field::Block},
	{{MagicType::Kind::Block, "timestamp"}, CallStateUtilities::Field::Block},
	{{MagicType::Kind::Message, "sender"}, CallStateUtilities::Field::Sender},
	{{MagicType::Kind::Message, "value"}, CallStateUtilities::Field::Value},
    {{MagicType::Kind::Transaction, "origin"}, CallStateUtilities::Field::Origin}
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

	auto const RES = CallStateUtilities::MAGIC_TYPE_LOOKUP.find({
        MAGIC_TYPE->kind(), _field
    });
	if (RES == CallStateUtilities::MAGIC_TYPE_LOOKUP.end())
	{
		throw runtime_error("Unable to resolve member of Magic type.");
	}
	return RES->second;
}

// -------------------------------------------------------------------------- //

string CallStateUtilities::get_name(CallStateUtilities::Field _field)
{
    // TODO: these should be escaped...
    // TODO: escaping names shouldn't belong to variable scope resolver...
    string retval = "unknowntype";
    if (_field == CallStateUtilities::Field::Block)
    {
        retval = "blocknum";
    }
    else if (_field == CallStateUtilities::Field::Sender)
    {
        retval = "sender";
    }
    else if (_field == CallStateUtilities::Field::Value)
    {
        retval = "value";
    }
    else if (_field == CallStateUtilities::Field::Paid)
    {
        retval = "paid";
    }
    else if (_field == CallStateUtilities::Field::Origin)
    {
        retval = "origin";
    }
    return retval;
}

// -------------------------------------------------------------------------- //

Type const* CallStateUtilities::get_type(CallStateUtilities::Field _field)
{
    switch (_field)
    {
    case CallStateUtilities::Field::Block: return &COUNTABLE_TYPE;
    case CallStateUtilities::Field::Sender: return &SENDER_TYPE;
    case CallStateUtilities::Field::Origin: return &SENDER_TYPE;
    case CallStateUtilities::Field::Value: return &COUNTABLE_TYPE;
    case CallStateUtilities::Field::Paid: return &BOOLEAN_TYPE;
    default: return nullptr;
    };
}

// -------------------------------------------------------------------------- //

}
}
}
