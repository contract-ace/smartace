// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Ensures Seahorn can reasonable about using calls, at least in some simple
 * case.
 */

library IntLib {
	function lessThan(uint256 a, uint256 b) internal pure returns (bool) {
		return (a < b);
	}
}

contract UsingLib {
	using IntLib for uint256;

	function f(uint256 a) public {
		assert(a.lessThan(10));
	}
}

