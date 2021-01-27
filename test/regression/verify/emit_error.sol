// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Ensures that side-effects of emit calls are observed.
 */

contract A {
    event TestEvent(uint a);
    uint i = 0;
    function f() internal returns (uint) {
        i = i + 1;
        return i;
    }
    function g() public {
        emit TestEvent(f());
    }
    function h() public {
        assert(i == 0);
    }
}
