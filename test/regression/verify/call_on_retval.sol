// RUN: %solc %s --bundle=C --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * A function cal return a contract by referene. This ensues that a call can be
 * made against a returned contract, and that the call is against the original
 * instance.
 */

contract A {
	int _x;

	function get() public view returns (int) { return _x; }

	function set(int x) public { _x = x; }
}

contract B {
	A _a;

	constructor() public { _a = new A(); }

	function get() public view returns (A) { return _a; }
}

contract C {
	B _b;

	constructor() public { _b = new B(); }

	function test() public {
		int oldx = _b.get().get();
		_b.get().set(oldx + 1);
		int newx = _b.get().get();
		assert(oldx + 1 == newx);
	}
}
