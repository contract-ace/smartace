// Copyright 2017-2019 ConsenSys AG
//
// RUN: %solc %s --aux-users=1 --lockstep-time=off --c-model --output-dir=%t --bundle Foo
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 0 0 2 1 0 0 1 2 1 1 0 0 1 2 1 1 0 0 1 2 1 1 0 0 1 2 0 1 0 0 1 2 1 1 0 0 0 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 6

/*
 * Regression test to ensure we can emulate contracts with binary inputs, and
 * integer output values.
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

