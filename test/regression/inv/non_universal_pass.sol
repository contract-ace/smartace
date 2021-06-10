// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * If a singleton (or role based) invariant is used, then address(0),
 * address(5), and address(this) will all be unable to write to m, leading to
 * the assertions to pass.
 */

contract A {
    mapping(address => int) m1;
    mapping(address => mapping(address => int)) m2;
    function f() public {
        require(msg.sender != address(5));
        m1[msg.sender] = m1[msg.sender] + 1;
    }
    function g(address a) public {
        require(msg.sender != address(5));
        require(a != address(0));
        require(a != address(5));
        require(a != address(this));
        m2[msg.sender][a] = m2[msg.sender][a] + 1;
    }
    function h() public {
        assert(m1[address(0)] == 0);
        assert(m1[address(5)] == 0);
        assert(m1[address(this)] == 0);
        assert(m2[address(0)][address(0)] == 0);
        assert(m2[address(0)][address(5)] == 0);
        assert(m2[address(0)][address(this)] == 0);
        assert(m2[address(5)][address(0)] == 0);
        assert(m2[address(5)][address(5)] == 0);
        assert(m2[address(5)][address(this)] == 0);
        assert(m2[address(this)][address(0)] == 0);
        assert(m2[address(this)][address(5)] == 0);
        assert(m2[address(this)][address(this)] == 0);
    }
}
