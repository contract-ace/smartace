// RUN: %solc %s --c-model --bundle C --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * The program assumes C will call down to constructor of A, and that A will run
 * the version of f defined in B. This is an unsafe implementation.
 *
 * This test follows from a bug in which A would call A.f() rather than B.f().
 */

contract A {
	int a;
	function f(int _a) internal { a = _a; }
	constructor() public {
		f(10);
		assert(a == 10);
	}
}

contract B is A {
    function f(int _a) internal { a = _a * 2; }
	constructor() public A() {}
}

contract C is B {
    constructor() public B() {}
    function f() public {}
}
