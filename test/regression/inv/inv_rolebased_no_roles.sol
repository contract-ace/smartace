// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type rolebased
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * If there are no roles, then role guards should default to true. Otherwise,
 * all executions of the program are blocked and this test would be safe.
 */

contract A {
    mapping(address => int) m1;

    function fail() public { assert(false); }
}
