// RUN: %solc %s --bundle=B --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * A contract must resolve functions relative to its parent after calling into a
 * super method.
 */

contract A {
	function val_2() public pure returns (int) { return 10; }
	function val() public pure returns (int) { return val_2(); }
}

contract B is A {
	function val_2() public pure returns (int) { return 9; }
	function val() public pure returns (int) { return super.val(); }
	function test() public pure { assert(val() == 9); }
}
