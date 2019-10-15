// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify | OutputCheck %s --comment=//
// CHECK: ^sat$

/*
 * Regression test for most basic behaviour of the seahorn integration. This
 * contract requires multiple iterations to reach the assertion. This ensures
 * that the verification model has at lesat some integraiton with seahorn.
 */

contract Contract {
	uint256 counter;
	function incr() public {
		counter = counter + 1;
		assert(counter < 2);
	}
}

