// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked --invar-infer=on
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * The safety of this program requires an adequate invariant for both m1 and m2.
 */

contract A {
    mapping(address => int) m1;
    mapping(address => int) m2;

    function f1() public {
        require(m1[msg.sender] <= 100);
        m1[msg.sender] += 1;
    }

    function f2() public {
        require(m2[msg.sender] <= 150);
        m2[msg.sender] += 1;
    }

    function g1() public {
        require(m1[msg.sender] >= -100);
        m1[msg.sender] -= 1;
    }

    function g2() public {
        require(m2[msg.sender] >= -150);
        m2[msg.sender] -= 1;
    }

    function check() public {
        assert(m1[msg.sender] >= -150);
        assert(m1[msg.sender] <= 150);
        assert(m2[msg.sender] >= -200);
        assert(m2[msg.sender] <= 200);
    }
}
