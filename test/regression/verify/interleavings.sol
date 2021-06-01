// RUN: %solc %s --c-model --output-dir=%t --bundle C
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * Regression test for transaction interleavings. Violation of the assertion
 * requires an interleaving of three calls, whose actions effect two values.
 * The first step depends on a specific argument being passed.
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

