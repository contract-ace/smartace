// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * The syntax here is ambiguous out of context. Without knowing that,
 * 1. g() is of type int256
 * 2. op is a library method imported through a using directive
 * it may appear tha g() is a contract on which op() is called, or that g is a
 * modifier of the op() method, called agains some missing base contract. This
 * is a regression test, as originally this case was missed, and he call
 * analyzer would expect an argument to g(), triggering a segfault.
 */

library Lib {
	function op(int256 x, int256 y) internal pure returns (bool) {
		return x != y;
	}
}

contract A {
	using Lib for int256;

	function g() public pure returns (int256) {
		return 10;
	}

	function f(int256 x) public pure {
		require(x != 10);
		assert(g().op(x));
	}
}

