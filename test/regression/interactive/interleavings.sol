// RUN: %solc %s --aux-users=1 --lockstep-time=off --c-model --output-dir=%t --bundle C
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 0 0 2 1 0 0 0 2 42 1 0 0 1 2 1 0 0 2 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 3

/*
 * Regression test for transaction interleavings. Violation of the assertion
 * requires an interleaving of three calls, whose actions effect two values.
 * This checks that the interactive model performs these transactions properly.
 */

contract C {
	int256 private x;
	int256 private y;

	function UpdateX(int _v) public {
		if (_v == 42) {
			x = 42;
		}
	}

	function UpdateY() public {
		if (x == 42) {
			y = 42;
		}
	}

	function CheckAssert() public {
		assert(x != 42 || y != 42);
	}
}

