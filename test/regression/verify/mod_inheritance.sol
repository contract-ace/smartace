// RUN: %solc %s --bundle=B --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Ensures that modifiers are overridden and executed correctly.
 */

contract A {
    uint public _a;
    modifier mod() {
        _a += 1;
        _;
    }
    function f() mod() public {}
}

contract B is A {
    modifier mod() {
        _a += 4;
        _;
    }
    function f() mod() public {
        super.f();
        assert(_a == 8);
        _a = 0;
    }
    function g() public {
        super.f();
        assert(_a == 4);
        _a = 0;
    }
}
