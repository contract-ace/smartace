// RUN: %solc %s --bundle=B --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Ensures that super calls call back into the base class.
 */

 contract A {
    int x;

    function f() public view { assert(x == 10); }

    function g() public {
        x = x + 1;
        f();
    }
}

contract B is A {
    bool called;

    function f() public view {
        require(called);
        assert(x == 9);
    }

    function g() public {
        called = true;
        x = 8;
        super.g();
    }
}
