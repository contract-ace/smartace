// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Securify seemingly fails to detect this case. f1 and f2 should be unsafe,
 * given their own specifications.
 *
 * This contract made use of a function pointer. We do not yet support function
 * pointers, but in this context, it is equivalent to a branch.
 *
 * TODO: restore function pointer
 * TODO: determine why securify claims there are no inputs to validate
 */

contract A {
    uint a;
    function f1(uint i) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_i = false;
        // INSTRUMENTATION (END)
        // INSTRUMENTATION (CHECK BEFORE WRITE)
        assert(_checked_i);
        a = i;
        // INSTRUMENTATION (END)
    }
    function f2(uint i) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_i = false;
        // INSTRUMENTATION (END)
        // INSTRUMENTATION (CHECK BEFORE WRITE)
        assert(_checked_i);
        a = i;
        // INSTRUMENTATION (END)
    }
    function g(uint i) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_i = false;
        // INSTRUMENTATION (END)
        // INSTRUMENTATION (CONTROL FLOW : TERNARY OPERATOR)
        // QUESTION: does Solidity encode ternary operations to jumps?
        // QUESTION: regardless, should ternary operations count?
        _checked_i = true;
        if (i < 100) {
            f1(i);
        }
        else {
            f2(i);
        }
    }
    function h() public {
        msg.sender.transfer(a);
    }
}
