// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Literal addresses should map to distinct values.
 */

contract A {
    function f() public {
        require(address(1) != address(2));
        require(address(1) != address(3));
        require(address(2) != address(3));
    }
}
