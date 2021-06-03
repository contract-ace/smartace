// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked --invar-infer=on
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: sat

/*
 * This program permits a trivial solution (true) so it should always pass.
 */

contract A {
    mapping(address => int) m1;
    mapping(address => int) m2;
    mapping(address => int) m3;
    function f(address a1, address a2) public {
        m1[a1] += 1;
        m2[a2] += 1;
        m3[msg.sender] += 1;
    }
}
