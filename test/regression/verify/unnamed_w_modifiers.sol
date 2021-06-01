// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Regression test for modifiers.
 */

contract A {
    modifier mod { require(false); _; }
    function f(int) public mod { assert(false); }
}