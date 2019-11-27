// RUN: %solc %s --c-model --map-k=1 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel
// RUN: echo 1 100 1 0 1 1 0 0 10 1 1 0 1 10 1 2 0 0 10 1 2 0 1 10 5 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 4

/**
 * Ensures that a 1 element map model works.
 */

contract OneMap {
    mapping(address => int) db;

    function write(int val) public {
        db[msg.sender] = val;
    }

    function check(int val) public view {
        assert(db[msg.sender] == val);
    }
}

