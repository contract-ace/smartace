// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * Ensures that fallbacks work.
 */

contract A {
	function () external payable {
        assert(false);
    }
}
