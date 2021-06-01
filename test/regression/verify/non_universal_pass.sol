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
    mapping(address => int) m;
    function f() public {
        require(msg.sender != address(5));
        m[msg.sender] = m[msg.sender] + 1;
    }
    function g() public {
        assert(m[address(0)] == 0);
        assert(m[address(5)] == 0);
        assert(m[address(this)] == 0);
    }
}
