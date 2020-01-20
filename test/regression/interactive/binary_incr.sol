// Copyright 2017-2019 ConsenSys AG
//
// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 100 1 0 0 1 1 0 0 1 1 1 1 0 0 1 1 1 1 0 0 1 1 1 1 0 0 1 0 1 1 0 0 1 1 1 1 0 0 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
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

