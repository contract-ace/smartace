// RUN: %solc %s --c-model --bundle B --output-dir=%t --allow-fallbacks on
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * If fallbacks are handled incorrectly, then this program is safe. After a call
 * to `B.f()`:
 * 1. unlocked is asserted (and succeeds),
 * 2. unlocked is set to false,
 * 3. a.f() is called,
 * 4. A transfers funds to owner (contract B),
 * 5. The fallback of contract B calls f(),
 * 6. assert(unlocked) fails.
 */

contract A {
	address payable owner;

	constructor() public {
		owner = msg.sender;
	}

	function f() public payable {
		owner.transfer(msg.value);
	}
}

contract B {
	A a;
	bool unlocked = true;

	constructor() public {
		a = new A();
	}
	
	function f() public payable {
		assert(unlocked);
		unlocked = false;
		a.f();
		unlocked = true;
	}

	function() external payable {
		f();
	}
}
