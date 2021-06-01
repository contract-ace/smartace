// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Tests that the verifier can reasoan about payments "to null".
 */

contract A {
    function f() public payable {
        assert(address(this).balance >= msg.value);
        msg.sender.transfer(address(this).balance);
        assert(address(this).balance == 0);
    }
}
