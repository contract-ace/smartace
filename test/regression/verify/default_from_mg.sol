// RUN: %solc %s --c-model --output-dir=%t --bundle Example
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * This test ensures that default initialization from global values works.
 */

contract Example {
    address a1 = msg.sender;
    address a2;

    constructor() public { a2 = msg.sender; }
    function check() public { assert(a1 == a2); }
}
