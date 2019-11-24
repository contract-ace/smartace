// RUN: %solc %s --c-model --contract-list B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This is testing that the verification engine can reason about the balance
 * mechanisms.
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
        require(msg.value > 0);
        a.f.value(msg.value)();
    }
    function g() public view {
        assert(address(this).balance == 0);
    }
}
