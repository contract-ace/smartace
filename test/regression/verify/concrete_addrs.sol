// RUN: %solc %s --c-model --concrete --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Ensures that enabling concrete maps disables interference.
 */

contract C {
	address _a;
	address _b;

	function set(address a, address b) public {
		a = _a;
		b = _b;
	}

	function check() public view {
		assert(_a == _b);
	}
}

