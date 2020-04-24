// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 2 0 0 5 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert

/**
 * Requires that ctors can handle modifiers.
 */

contract ModifierContract {
	modifier m0() {
		assert(msg.value != 5);
		_;
	}

	constructor() public payable m0() { }
}
