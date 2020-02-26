// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
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

