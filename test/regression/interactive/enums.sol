// RUN: %solc %s --c-model --lockstep-time=off --aux-users=1 --output-dir=%t --bundle A
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 0 0 2 1 0 0 2 0 1 0 0 2 1 1 0 0 2 2 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 3

/**
 * Ensures that a 1 element map model works.
 */

contract A {
    enum MyEnum { A, B, C }
    MyEnum _x;

    function test(MyEnum x) public {
        _x = x;
        assert(_x != MyEnum.C);
    }
}

