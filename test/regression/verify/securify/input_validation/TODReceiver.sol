// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This example is trivial, but is still useful for reference if we were to add
 * validation tests to the set of properties we verify.
 */

contract A {
    address payable winner;
    function play(uint guess) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_guess = false;
        // INSTRUMENTATION (END)
        // INSTRUMENTATION (CHECK BEFORE BRANCH)
        _checked_guess = true;
        if (guess == 0xDEADBEEF) {
        // INSTRUMENTATION (END)
            winner = msg.sender;
        }
    }
    function getReward() public payable {
        winner.transfer(msg.value);
    }
}
