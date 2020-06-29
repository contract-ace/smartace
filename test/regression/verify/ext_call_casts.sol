// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * When testing with Melon, I hit a case where devirtualization of inherited
 * methods failed. This is a minimal example of the case.
 */

interface IBase {
    function f() external pure returns (uint256);
}

contract Base is IBase {
    function f() external pure returns (uint256) { return 100; }
}

contract Derived is Base {
    function g() external view returns (uint256) { return this.f(); }
}

contract Caller {
    Derived d;
    constructor() public { d = new Derived(); }
    function f() public view { assert(d.f() == d.g()); }
}
