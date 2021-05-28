// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * In this case the owner is bypassed.
 */

contract A {
    bool called;
    function f() public payable {
        called = true;
    }
    function g() public view {
        require(called);
        assert(address(this).balance > 0);
    }
}

contract B {
    A a;
    constructor() public { a = new A(); }
    function f() public payable {
        a.f.value(msg.value)();
    }
    function g() public view {
        assert(address(this).balance == 0);
    }
}
