// RUN: %solc %s --reps=1 --lockstep-time=off --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 0 0 2 1 0 0 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/**
 * Requires that chained modifiers are supported properly (branching).
 */

contract ModifierContract {
	int x;

	modifier m0() {
		x = x + 1;
		_;
		_;
		assert(x != 3);
	}

	function noop() public m0() m0() { }
}
