// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Regression test for diamond inheritance.
 */

contract A {
    uint public _a = 0;
}

contract B is A {
    constructor() public { _a += 1; }
}

contract C is A {
    constructor() public { _a += 1; }
}

contract D is B, C {
    function f() public {
        assert(_a == 2);
    }
}
