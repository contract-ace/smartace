// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DCMAKE_CXX_COMPILER=%clangpp -DCMAKE_C_COMPILER=%clangc -DSEA_PATH=%seapath -DFUZZ_RUNS=5000 %buildargs
// RUN: cmake --build . --target fuzz

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
