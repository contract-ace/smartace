// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Ensures that getters work.
 */

contract A {
	uint public x;
	uint public y;

	function setX(uint _x) public { x = _x; }
	function setY(uint _y) public { y = _y; }
}

contract B {
	A a;
	constructor() public { a = new A(); }
	function f() public view returns (uint x, uint y) {
		x = a.x();
		y = a.y();
	}
	function g() public {
		uint x;
		uint y;
		(x, y) = f();
		assert(x == a.x());
		assert(y == a.y());
	}
}

