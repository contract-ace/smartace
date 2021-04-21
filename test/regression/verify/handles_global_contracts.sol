// RUN: %solc %s --c-model --allow-fallbacks on --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * This example triggers global contract variables by:
 * 1. having a call to send;
 * 2. having a fallback;
 * 3. having global fallbacks enabled.
 * The fallback is semantically unreachable.
 */

contract A {
    int counter = 0;
    function g() internal {
        if (counter < 100) {
            counter = counter + 1;
        }
    }
    function() external payable {
        g();
    }
    function f() public payable {
        msg.sender.transfer(msg.value);
    }
    function h() public {
        assert(counter <= 100);
    }
}
