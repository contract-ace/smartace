
// RUN: %solc %s --c-model --contract-list B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 10 20 1 0 0 1 1 10 0 0 | ./icmodel 2>&1

/*
 * Tests that payments forward correctly.
 */

contract A {
    function f() public payable {
        assert(msg.value > 0);
        assert(address(this).balance > 0);
    }
}

contract B {
    A a;
    constructor() public { a = new A(); }
    function f() public payable {
        assert(msg.value > 0);
        a.f.value(msg.value)();
        assert(address(this).balance == 0);
    }
}
