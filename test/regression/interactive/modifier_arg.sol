// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 100 1 0 1 1 0 0 1 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/**
 * Requires that modifiers handle args appropriately.
 */

contract ModifierContract {
	modifier m0(int x, int y) {
		x = 0;
		y = 0;
		require(x == 0);
		require(y == 0);
		_;
	}

	function f(int x, int y) public m0(y, x) {
		assert(x >= y);
	}
}
