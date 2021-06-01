// RUN: %solc %s --c-model --lockstep-time=off --output-dir=%t --bundle Contract
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel

/*
 * This contract doesn't do much, but it uses some syntactic sugar provided by
 * Solidity. It is here to ensure that these features do not break accidentally
 * between refactors. This test is considered passing if it compiles. If it
 * still compiles, then presumably the syntactic sugar is still handled, and is
 * still reduced to code which is verified in other tests.
 */

contract Contract {
	function unnamed_args_named_rv_call(uint, uint) public pure returns (uint a) {
		a = 5;
	}
}

