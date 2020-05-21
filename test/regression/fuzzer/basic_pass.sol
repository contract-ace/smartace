// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make fuzz

/*
 * Regression test for most basic behaviour of the fuzzer. This contract has one
 * funciton without an assertion, so if this test fails, it is a true bug in the
 * verification framework.
 */

contract Contract {
	uint256 counter;
	function incr() public {
		counter = counter + 1;
	}
}

