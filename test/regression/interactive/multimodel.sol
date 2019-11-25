// RUN: %solc %s --c-model --contract-list A A B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 10 20 30 1 0 0 1 0 0 1 0 0 1 1 0 0 0 1 1 1 0 0 1 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// RUN: echo 1 10 20 30 1 0 0 1 0 0 1 0 0 1 1 0 0 2 1 1 1 0 0 3 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// RUN: echo 1 10 20 30 1 0 0 1 0 0 1 0 0 1 1 0 0 4 2 1 1 0 0 5 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 2

/*
 * Tests that model permits interactions with each contract.
 */

contract A {
    bool flag = false;
    function f(int a) public { if (a == 1) flag = true; }
    function g() public view { assert(!flag); }
}

contract B {
    bool flag = false;
    function f(int a) public { if (a == 2) flag = true; }
    function g() public view { assert(!flag); }
}
