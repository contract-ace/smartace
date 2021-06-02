// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/*
 * If a universal invariant is used, then address(0), address(5), and
 * address(this) can spuriously change values.
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
