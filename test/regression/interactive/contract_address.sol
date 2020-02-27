// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 0 1 0 0 1 1 0 0 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/*
 * Ensures that at a minimal, addresses are tracked on contracts.
 */

contract A {
    function fails() public view {
        assert(address(this) != address(10));
    }
}
