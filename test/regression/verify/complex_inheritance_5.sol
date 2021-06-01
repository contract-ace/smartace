// RUN: %solc %s --c-model --output-dir=%t --bundle G
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * Regression test for complex inheritance.
 */

contract A {
    uint public a;
    constructor(uint _a) public { a = _a; }
}

contract B is A {
    constructor(uint _a) public A(_a + 1) {}
}

contract C is A {
    uint public b = 5;
}

contract D {
    uint public c;
    constructor(uint _c) public { c = _c; }
}

contract E is D {}

contract F {
    uint d;
    constructor(uint _d) public { d = _d; }
}

contract G is B, C, E, F(20) {
    constructor() B(7) D(10) public {}

    function f() public {
        assert(a == 7);
        assert(b == 5);
        assert(c == 10);
        assert(d == 19);
    }
}
