// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

// This contract implements a common crowdfunding dapp. It follows a three mode
// model, as described below.
//   INVEST_PHASE: (now <= end)
//   OPTOUT_PHASE: (now > end) && (target < balance)
//   PAYOUT_PHASE: (now > end) && (target >= balance)
// This contract is manually implemented to achieve a defensive check. A
// defensive check passes if under any sequence of transactions, the system
// remains in one of its possible modes.
//
// This test checks the reachability of a state in which the conjunction of all
// modes is false.
//
// In this implementation, the modes have been implemented correctly, and a
// defensive check should pass.
//
// The proof of this property is independant of the investment map.
contract Crowdfund {
	uint end;
	uint target;
	address payable dest;

	uint balance;
	mapping(address => uint) investments;

	constructor(uint _end, uint _target, address payable _dest) public {
		end = _end;
		target = _target;
		dest = _dest;
	}

	function invest() public payable {
		require(now <= end);
		investments[msg.sender] += msg.value;
		balance += msg.value;
	}

	function opt_out() public {
		require(now > end && balance < target);
		uint amt = investments[msg.sender];
		investments[msg.sender] = 0;
		balance -= amt;
		msg.sender.transfer(amt);
	}

	function claim() public {
		require(now > end && balance > target);
		balance = 0;
		dest.transfer(balance);
	}

	function _defensive_check() public view {
		bool in_invest_phase = (now <= end);
		bool in_optout_phase = ((now > end) && (target < balance));
		bool in_payout_phase = ((now > end) && (target >= balance));
		assert(in_invest_phase || in_optout_phase || in_payout_phase);
	}
}

