// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * If contract A calls contract B, then contract B should get contract A's
 * address. This is tested by looking at the owner paradigmn.
 *
 * This case checks that owner can bind to a contract. If that contract never
 * calls an owner method, it is not callable.
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
        require(false);
        a.bad();
    }
}
