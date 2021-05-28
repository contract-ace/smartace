// RUN: %solc %s --c-model --bundle B --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * The model should expose Method_B_Funcf from specializing the inherited member
 * A.f(). This method is safe wrt. A, however, wrt. B setting field a to 15
 * should result in an assertion failure.
 */

contract A {
    int a = 10;
    function f() public view {
        assert(a != 10);
    }
}

contract B is A {
    int b = 15;
}
