// RUN: %solc %s --c-model --map-k=1 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 100 1 0 0 1 1 5 0 0 1 1 5 0 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 2

/**
 * Ensures that sending a value updates the contract balance.
 */

contract OneMap {
    function checkbal() public payable {
        assert(address(this).balance < 10);
    }
}

