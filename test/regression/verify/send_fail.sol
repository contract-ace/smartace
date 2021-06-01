// RUN: %solc %s --c-model --output-dir=%t --bundle AssertOnFail
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Ensures that under certain circumstances, send can fail.
 */

contract AssertOnFail {
	function f(address payable _a) public {
		if (!_a.send(0)) { assert(false); }
	}
}
