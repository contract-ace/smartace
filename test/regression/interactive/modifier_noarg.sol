// RUN: %solc %s --reps=1 --lockstep-time=off --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 3 2 0 0 1 2 0 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/**
 * Requires that modifiers handle empty arg lists appropriately.
 */

contract ModifierContract {
	modifier m0() {
		assert(msg.sender == address(2) || address(this) == address(2));
		_;
	}

	function f() public view m0() {
		return;
	}
}
