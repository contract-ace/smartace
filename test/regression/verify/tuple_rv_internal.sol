// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Tests internal RV's, with and without excluded destinations.
 */

contract A {
    function f(int a) internal pure returns (int i, int j, int k) {
        i = a - 1;
        j = a;
        k = a + 1;
    }

    function g(int a) external pure {
        int i_1;
        int j_1;
        int k_1;
        int i_2;
        int j_3;
        int k_4;

        (i_1, j_1, k_1) = f(a);
        (i_2, ,) = f(a);
        (, j_3,) = f(a);
        (, , k_4) = f(a);

        assert(i_1 == a - 1);
        assert(j_1 == a);
        assert(k_1 == a + 1);

        assert(i_1 == i_2);
        assert(j_1 == j_3);
        assert(k_1 == k_4);
    }
}

