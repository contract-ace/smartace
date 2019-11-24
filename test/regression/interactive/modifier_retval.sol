// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 100 1 0 0 1 1 0 0 1 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/**
 * Requires that modifiers handle return values appropriately.
 */

contract ModifierContract {
	int x;

	modifier m0() {
		_;
		_;
		_;
	}

	function incr() public m0() returns (int) {
		x = x + 1;
		return x;
	}

	function call_noop() public { assert(incr() != 3); }
}
