// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DINT_MODEL=USE_BOOST_MP -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DSEA_PATH=%seapath
// RUN: cmake --build . --target fuzz
// XFAIL: true

/**
 * Regression test for (1) compiling with big ints and (2) compiling with big
 * ints. This test relies on overflow being inevitable in this setup.
 */

contract Contract {
	int8 counter;
	function incr() public {
        int8 old = counter;
		counter = old + 127;
        assert(counter > old);
	}
}
