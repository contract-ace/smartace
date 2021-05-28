// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * This test is meant to check that given a lib with method f(Struct storage),
 * it is possible to call x.f() and f(x). This was a bug in an earlier version.
 */

library Lib {
	struct S { bool v; }

	function set(S storage x) internal {
		require(!check(x), "");
		x.v = true;
	}

	function check(S storage x) internal view returns (bool) {
		return x.v;
	}
}

contract Test {
	using Lib for Lib.S;

	Lib.S x;

	function test() public {
		x.set();
		assert(!x.check());
	}
}
