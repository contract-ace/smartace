// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Requires that unnamed tuple RV's work.
 */

contract Contract {
    function f() public pure returns (uint, uint) {
        return (4, 5);
    }
    function g() public pure {
        uint x;
        uint y;
        (x, y) = f();
        assert(x == 4);
        assert(y == 5);
    }
}
