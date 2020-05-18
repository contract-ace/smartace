// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * This checks that a counterexample calling A and B is obtainable.
 */

contract A {
    int x = 5;
    function setX(int _x) public { x = _x; }
    function getX() public view returns (int) { return x; }
}

contract B {
    A a;
    constructor() public { a = new A(); }
    function check() public view { assert(a.getX() != 6); }
}
