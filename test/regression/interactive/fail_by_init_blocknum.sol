// RUN: %solc %s --reps=1 --lockstep-time=off --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 10 100 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 0

/*
 * This simple contract can fail an assert on construction, but this assert is
 * only considered if arbitrary blocknumbers can be set on construction.
 */

contract FailByInitBlocknum {
	constructor() public {
		assert(block.number != 10);
	}
}
