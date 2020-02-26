// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * The model should prevent contracts from generating calls.
 */

contract Crowdfund {
	function call() public {
		assert(msg.sender != address(this));
	}
}

