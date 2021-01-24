// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Fails if multiple functions with overrides are resolved improperly.
 */

contract A {
    uint _a;
    constructor(uint a) public { _a = a; }
     function f() internal view returns (uint) { return _a; }
}

contract B is A {
    constructor() public {}
    function f() internal view returns (uint) { return super.f() + 5; }
}

contract C is A {
    constructor() public {}
    function f() internal view returns (uint) { return super.f() + 2; }
}

contract D is C {}

contract E is B, D {
    constructor() public A(10) B() C() {}
    function check() public view { assert(f() == 17); }
}
