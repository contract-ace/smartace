// RUN: %solc %s --aux-users=1 --lockstep-time=off --c-model --output-dir=%t --bundle ModifierContract
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 3 0 0 2 1 0 0 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
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
