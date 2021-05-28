// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This example is trivial, but is still useful for reference if we were to add
 * validation tests to the set of properties we verify.
 *
 * We do not yet support the exponentiation operator.
 *
 * TODO: restore ** operator
 */

contract A {
    bool won = false;
    function play(uint guess) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_guess = false;
        // INSTRUMENTATION (END)
        require(!won);
        // INSTRUMENTATION (CHECK BEFORE BRANCH)
        _checked_guess = true;
        if (guess == 0xDEADBEEF) {
        // INSTRUMENTATION (END)
            won = true;
            msg.sender.transfer(10);
        }
    }
}
