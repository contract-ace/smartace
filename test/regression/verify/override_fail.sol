// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * A positive test for overriden functions.
 */

contract A {
    function f(int a) public pure returns (int _a) {
        _a = a + 10;
        assert(_a != 20);
    }
}

contract B is A {
    function f(int a) public pure returns (int _a) {
        _a = super.f(10) + a;
    }
}

