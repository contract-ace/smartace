// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DINT_MODEL=USE_BOOST_MP -DCMAKE_CXX_COMPILER=%clangpp -DCMAKE_C_COMPILER=%clangc -DSEA_PATH=%seapath %buildargs
// RUN: cmake --build . --target fuzz
// XFAIL: true

/**
 * Regression test for (1) compiling with big ints and (2) the ability for big
 * ints to overflow. This test relies on overflow being inevitable in this
 * setup.
 */

contract Contract {
	uint calls;
	int8 counter;
	function incr() public {
        int8 old = counter;
		counter = old + 127;
		calls = calls + 1;
        assert(calls == 2 && counter > old);
	}
}
