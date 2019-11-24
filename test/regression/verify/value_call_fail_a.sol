// RUN: %solc %s --c-model --contract-list B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * In this case not all balance is passed on.
 */

contract A {
    address owner;
    bool called;
    constructor() public { owner = msg.sender; }
    function f() public payable {
        require(msg.sender == owner);
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
        require(msg.value > 1);
        a.f.value(msg.value - 1)();
    }
    function g() public view {
        assert(address(this).balance == 0);
    }
}
