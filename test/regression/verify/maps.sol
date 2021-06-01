// RUN: %solc %s --c-model --output-dir=%t --bundle C
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: ^sat

/**
 * Ensures non-deterministic maps work.
 */

contract C {
    struct A {
        mapping(address => uint) m_1;
    }
    struct B {
        A a;
        mapping(address => uint) m_2;
    }
    B b;
    mapping(address => uint) m_3;
    function foo() public view {
        uint x = m_3[msg.sender];
        uint y = b.m_2[msg.sender];
        uint z = b.a.m_1[msg.sender];
        assert(x == y || x == z || y == z);
    }
}
