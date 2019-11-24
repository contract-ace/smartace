// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 10 20 1 0 0 1 1 10 0 0 0 | ./icmodel 2>&1

/*
 * Tests that payments to "the null" work.
 */

contract A {
    function f() public payable {
        assert(address(this).balance >= msg.value);
        msg.sender.transfer(address(this).balance);
        assert(address(this).balance == 0);
    }
}
