// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 100 10 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: require
// CHECK: Transaction Count: 0

/*
 * This simple contract can fail an assert on construction, but this assert is
 * only considered if arbitrary message senders can be set on construction.
 */

contract FailByInitValue {
	constructor() public payable {
		require(msg.sender != address(10));
	}
}
