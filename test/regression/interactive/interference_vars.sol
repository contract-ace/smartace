// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 2 0 0 1 2 0 0 0 1 1 1 2 0 0 1 2 0 0 0 3 4 2 0 0 1 3 3 0 | ./icmodel

/*
 * When a variabe is set to a representative address, this should be fixed. When
 * a variable is set to a representative address, it should be able to take a
 * unique interference address on each transaction. This tests both cases
 * manually.
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

