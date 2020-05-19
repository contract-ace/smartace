// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * If contract A calls contract B, then contract B should get contract A's
 * address. This is tested by looking at the owner paradigmn.
 *
 * This case checks that owner can bind to a contract. If that contract does
 * call an owner method, it can drive the system to failure.
 */

contract A {
    address owner;
    constructor() public { owner = msg.sender; }
    function bad() public view {
        require(msg.sender == owner);
        assert(false);
    }
}

contract B {
    A a;
    constructor() public { a = new A(); }
    function f() public view {
        a.bad();
    }
}
