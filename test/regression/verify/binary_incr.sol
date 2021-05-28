// Copyright 2017-2019 ConsenSys AG
//
// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * Regression test to ensure we can verify transactional sequences in which many
 * transaction are required to form a counter-example, and in which each such
 * transaction is parametric.
 */

contract Foo {
	int256 private x;

	function Bar() public view returns (int256) {
		if (x == 29) {
			assert(false);
			return 1;
		}
		return 0;
	}

	function SetNext(bool b) public {
		x = 2*x + (b ? 1 : 0);
	}
}

