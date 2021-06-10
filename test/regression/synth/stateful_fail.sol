// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked --invar-infer=on --invar-stateful=on
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK-NOT: unsat

/*
 * This program permits a trivial solution (true) so it should always pass.
 */

contract A {
    int v;
    mapping(address => int) m;
    function f() public {
        require(v < 10);
        v = v + 1;
    }
    function g() public {
        m[msg.sender] = v;
    }
    function h() public {
        assert(m[msg.sender] < 5);
    }
}
