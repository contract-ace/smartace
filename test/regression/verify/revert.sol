// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Solidity defines revert to behave similarly to require. If require is
 * implemented properly, it should therefore be able to block an assert.
 */

contract A {
    function foo(int _a) public pure {
        int a = _a;
        if (_a == 10) {
            revert("_a must not be 10!");
        }
        assert(a != 10);
    }
}
