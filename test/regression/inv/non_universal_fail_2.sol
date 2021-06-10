// RUN: %solc %s --c-model --output-dir=%t --bundle C --invar-type singleton
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * Ensures that enabling concrete maps disables interference.
 */

contract C {
    mapping(address => mapping(address => int)) m;

	function check() public view {
		assert(m[address(0)][address(0)] == m[address(0)][msg.sender]);
	}
}
