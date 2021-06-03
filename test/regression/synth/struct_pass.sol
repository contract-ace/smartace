// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked --invar-infer=on
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * This program permits a trivial solution (true) so it should always pass.
 */

contract A {
    struct S {
        int a;
        int b;
    }
    mapping(address => S) m;
    function f() public {
        S memory s = m[msg.sender];
        s.a += 1;
        s.b += 1;
        m[msg.sender] = s;
    }
    function g() public {
        assert(m[msg.sender].a == m[msg.sender].b);
    }
}
