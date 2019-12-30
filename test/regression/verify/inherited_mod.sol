// RUN: %solc %s --c-model --contract-list Child --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Requires that inherited modifiers are accessible to children.
 */

contract Parent {
    address owner;

	constructor() public {
        owner = msg.sender;
    }

    modifier ownerOnly() {
        require(msg.sender == owner);
        _;
    }
}

contract Child is Parent {
    function f() public view ownerOnly() {
        assert(msg.sender == owner);
    }
}
