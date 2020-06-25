// RUN: %solc %s --reps=1 --lockstep-time=off --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 2 0 0 1 2 0 0 1 2 0 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 2

/*
 * Regression test for most basic behaviour of the system. Ensures the
 * interactive model can hit the exception.
 */

contract Contract {
	uint256 counter;
	function incr() public {
		counter = counter + 1;
		assert(counter < 2);
	}
}

