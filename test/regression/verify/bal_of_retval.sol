// RUN: %solc %s --bundle=B --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Ensures that the balance of a contract returned by a function is accessible.
 */

contract A {
}

contract B {
	A a;

	constructor() public { a = new A(); }

	function get() public view returns (A) { return a; }

	function test() public view {
		assert(address(a).balance == address(get()).balance);
	}
}
