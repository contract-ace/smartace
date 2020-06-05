// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DSEA_PATH=%seapath
// RUN: cmake --build . --target fuzz
// XFAIL: true

/*
 * Regression test for most basic behaviour of the fuzzer. This contract has one
 * method to ensure that the fuzzer inevitably fails the assertion.
 */

contract Contract {
	uint256 counter;
	function incr() public {
		counter = counter + 1;
		assert(counter < 2);
	}
}

