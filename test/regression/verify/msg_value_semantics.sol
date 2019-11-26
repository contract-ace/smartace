// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * In a previous revision it was possible to "double spend" the ETH passed
 * through msg.value. This program is SAT if that bug is reintroduced.
 */

contract A {
    function g() public payable {
        return;
    }
    function f() public payable {
        uint a = address(this).balance;
        g();
        assert(a == address(this).balance);
    }
}
