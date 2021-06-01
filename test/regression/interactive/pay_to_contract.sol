// RUN: %solc %s --aux-users=1 --lockstep-time=off --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 0 0 2 1 0 0 2 1 1 0 | ./icmodel 2>&1

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
