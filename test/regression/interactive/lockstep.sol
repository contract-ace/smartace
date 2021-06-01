// RUN: %solc %s --c-model --lockstep-time=on --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 2 1 1 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// RUN: echo 2 1 1 3 1 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// RUN: echo 2 1 1 3 2 0 | ./icmodel
// CHECK: require
// CHECK: Transaction Count: 0

/**
 * Tests that lockstep enforces both values to increase.
 */

contract Lockstep {
    function f() public pure { return; }
}
