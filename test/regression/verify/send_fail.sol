// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Ensures that under certain circumstances, send can fail.
 */

contract AssertOnFail {
	function f(address payable _a) public {
		if (!_a.send(0)) { assert(false); }
	}
}
