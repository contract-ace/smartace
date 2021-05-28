// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * This test is meant to check that storage data is propogated correctly. This
 * test only compiles if we can implicitly pass a value by reference to a
 * library, and if it can be detected as a reference when taken as a storage
 * variable. The check that [pre(a.v) == a.v] only passes if the struct is
 * passed by value.
 *
 * Note that the relation between Lib and EndUser is idiomatic of Solidity
 * libaries which provide abstract data types.
 */

library Lib {
	struct A { int v; }

	function add(A storage a, int v) internal {
        A storage b = a;
        b.v = b.v + v;
    }
}

contract EndUser {
	using Lib for Lib.A;

	Lib.A a;

	function func() public {
		int pre_v = a.v;
		a.add(1);
		assert(pre_v == a.v);
	}
}
