// RUN: %solc %s --c-model --output-dir=%t --fail-on-require
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Failure escalation should propogate to library calls.
 */

library Lib {
    function sub(uint256 a, uint256 b) internal pure returns (uint256 c) {
        require(a >= b);
        c = a - b;
    }
}

contract C {
    using Lib for uint256;
    function f() public pure {
        uint256 a = 5;
        uint256 b = 6;
        a.sub(b);
    }
}
