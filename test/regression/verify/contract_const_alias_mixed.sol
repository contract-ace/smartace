// RUN: %solc %s --c-model --output-dir=%t --bundle Crowdfund
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * The model should prevent contracts from generating calls.
 */

contract Crowdfund {
	function call() public {
		assert(address(0) != address(this));
		assert(address(1) == address(this));
	}
}

