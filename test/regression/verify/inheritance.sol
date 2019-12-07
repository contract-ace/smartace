// RUN: %solc %s --c-model --contract-list B --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * The model should expose Method_B_Funcf from specializing the inherited member
 * A.f(). This method is safe wrt. A, however, wrt. B setting field a to 15
 * should result in an assertion failure.
 */

contract A {
    int a = 10;
    function f() public view {
        assert(a != 15);
    }
}

contract B is A {
    int a = 15;
}
