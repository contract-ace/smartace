// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * If a universal invariant is used, then address(0), address(5), and
 * address(this) can spuriously change values.
 */

contract A {
    mapping(address => int72) m1;
    mapping(address => uint24) m2;
    mapping(address => bool) m3;
    function f() public {}
}
