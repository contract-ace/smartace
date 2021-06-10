// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule unchecked
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * If a universal invariant is used, then address(0), address(5), and
 * address(this) can spuriously change values.
 */

contract A {
    mapping(address => int) m1;
    mapping(address => int) m2;
    mapping(address => int) m3;
    function f(address a1, address a2) public {
        require(msg.sender != address(404));
        require(msg.sender != address(42));
        m1[a1] = m1[a1] + 1;
        m2[a2] = m2[a2] + 1;
        m3[msg.sender] = m1[a1] + m2[a2];
    }
}
