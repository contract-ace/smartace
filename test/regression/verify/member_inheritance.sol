// RUN: %solc %s --c-model --output-dir=%t --bundle A B C
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This checks that parent constructor calls and variable scope overriding works
 * in a non-trivial case.
 */

contract A {
    int a = 10;
    int b = 20;
    int c;
    constructor(int _c) public {
        assert(a == 10);
        assert(b == 20);
        assert(c == 0);
        c = _c;
    }
}

contract B is A {
    int d = 10;
    constructor(int _a, int _d) public A(_a) {
        assert(a == 10);
        assert(b == 20);
        assert(c == _a);
        assert(d == 10);
        a = _a;
        d = _d;
    }
}

contract C is B {
    int e = 5;
    constructor(int _a) public B(_a, 20) {
        assert(a == _a);
        assert(b == 20);
        assert(c == _a);
        assert(d == 20);
        assert(e == 5);
    }
    function f() public {}
}
