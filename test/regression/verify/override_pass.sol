// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * A positive test for overriden functions.
 */

contract A {
    function f() public pure returns (int a) {
        a = 10;
        assert(a == 10);
    }
}

contract B is A {
    function f() public pure returns (int a) {
        a = super.f() + 10;
        assert(a == 20);
    }
}

contract C is B {
    function f() public pure returns (int a) {
        a = super.f() + 10;
        assert(a == 30);
    }
}
