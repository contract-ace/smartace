// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Checks that invariants are generated for structures.
 */

contract A {
    struct A {
        int x1;
        int x2;
        int x3;
    }
    struct B {
        A a1;
        A a2;
        int x1;
        int x2;
    }
    struct C {
        A a1;
        B b1;
        int x1;
    }
    mapping(address => C) m1;
    function f() public {}
}
