// RUN: %solc %s --c-model --contract-list B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 10 20 1 0 0 1 1 0 0 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// RUN: echo 1 10 20 1 0 0 1 1 0 0 1 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// RUN: echo 1 10 20 1 0 0 1 1 0 0 2 4 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/*
 * Tests that model permits interactions with each contract.
 */

contract A {
    int x;
    constructor(int _x) public { x = _x; }
    function f(int _x) public view { assert(x == _x); }
}

contract B {
    A a;
    constructor() public { a = new A(5); }
    function f() public view { assert(address(this) == address(a)); }
    function g() public view { assert(address(a).balance != 0); }
}
