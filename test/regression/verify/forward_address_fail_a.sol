// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * If contract A calls contract B, then contract B should get contract A's
 * address. This is tested by looking at the owner paradigmn.
 *
 * Sanity check for when bad() is not blocked.
 */

contract A {
    address owner;
    constructor() public { owner = msg.sender; }
    function bad() public view {
        assert(false);
    }
}

contract B {
    A a;
    constructor() public { a = new A(); }
    function f() public view {
        require(false);
        a.bad();
    }
}
