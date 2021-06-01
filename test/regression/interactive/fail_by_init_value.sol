// RUN: %solc %s --reps=1 --lockstep-time=off --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 100 10 2 10 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 0

/*
 * This simple contract can fail an assert on construction, but this assert is
 * only considered if arbitrary message values can be set on construction.
 */

contract FailByInitValue {
	constructor() public payable {
		assert(msg.value != 10);
	}

	function f() public {}
}
