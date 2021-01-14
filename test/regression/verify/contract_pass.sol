// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Regression test for internal contract passing.
 */

contract Other {
	uint256 public val;
	function f() public view returns (uint) {
		return val;
	}
}

contract Contract {
	Other other;
	constructor() public {
		other = new Other();
	}
	function g(Other o) internal returns (uint) {
		return o.f();
	}
	function f() public returns (uint) {
		return g(other);
	}
}

