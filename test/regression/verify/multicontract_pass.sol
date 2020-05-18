// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This checks that a counterexample calling A and B is obtainable.
 */

contract A {
    int x = 5;
    function setX(int _x) public {
        require(_x != 6);
        x = _x;
    }
    function getX() public view returns (int) { return x; }
}

contract B {
    A a;
    constructor() public { a = new A(); }
    function check() public view { assert(a.getX() != 6); }
}
