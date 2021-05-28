// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * There was a bug which reduced named, unset return values to non-determinism.
 */

contract ZeroRV {
    function g() internal pure returns (uint a) {
        a;
    }
	function f() public pure {
		assert(g() == 0);
	}
}
