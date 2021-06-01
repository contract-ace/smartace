// RUN: %solc %s --c-model --output-dir=%t --bundle Crowdfund
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * In Ethereum, the 0 address is reserved, and is not held by any sender. This
 * contract is safe if the 0 address restriction is respected.
 */

contract Crowdfund {
	function call() public {
		assert(msg.sender != address(0));
	}
}

