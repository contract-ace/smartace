// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Ensures that enums are in range and can be cast.
 */

contract A {
    enum ValueId { Value1, Value2, Value3 }
    function f(ValueId id) public pure {
        if (id == ValueId.Value1) {
            assert(uint(id) == 0);
        }
        else if (id == ValueId.Value2) {
            assert(uint(id) == 1);
        }
        else if (id == ValueId.Value3) {
            assert(uint(id) == 2);
        }
        else {
            assert(false);
        }
    } 
}
