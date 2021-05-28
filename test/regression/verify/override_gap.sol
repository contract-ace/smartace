// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * A positive test that super calls are distinguishable.
 */

contract A {
    function f() public pure returns (int a) { a = 5; }
    function g() public pure returns (int a) { a = 10; }
}

contract B is A {
    function h() public pure {
        assert(f() != g());
    }
}
