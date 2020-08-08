// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * In Solidity, tuple assignments should act as temporary values. The code given
 * should implement an atomic rotate.
 */

contract A {
    function f() public pure {
        int i = 0;
        int j = 1;
        int k = 2;

        int old_i = i;
        int old_j = j;
        int old_k = k;

        (i, j, k) = (k, i, j);

        assert(i == old_k);
        assert(j == old_i);
        assert(k == old_j);
    }
}
