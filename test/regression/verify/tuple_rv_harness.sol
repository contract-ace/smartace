// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * The harness should set up tuple return values appropriately.
 */

contract A {
    function f(int a) public pure returns (int i, int j, int k) {
        i = a - 1;
        j = a;
        k = a + 1;
        assert(j == a);
    }
}

