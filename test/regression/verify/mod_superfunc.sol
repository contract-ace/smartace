// RUN: %solc %s --bundle=B --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Ensures that modifiers are called on super methods.
 */

contract A {
    uint public _a;
    modifier mod1() {
        _a += 1;
        _;
    }
    function f() mod1() public {}
}

contract B is A {
    modifier mod2() {
        _a += 4;
        _;
    }
    function f() mod2() public {
        super.f();
        assert(_a == 5);
        _a = 0;
    }
}
