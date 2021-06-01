// RUN: %solc %s --c-model --output-dir=%t --bundle C
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify | OutputCheck %s --comment=//
// RUN: make cex
// [ ! -f cex.ll ]
// CHECK: ^unsat$

/*
 * Regression test for most basic contract features. This contract is UNSAT if
 * basic contract and transactional semantics are encoded.
 */

contract C {
	int256 private x;
	int256 private y;

	constructor () public {
		// Feature: Constructor => State(0).
		x = 0;
		y = 1;
	}

	function Incr() public {
		if (x < 32) {
			x += 2;
			y += 2;
		} else {
			// Feature: transactional semantics (rollback).
      			x++;
			require(false);
		}
	}

	function Foo() public view {
		// Feature: extract invariant that x % 2 == 0.
		assert(x != 3);
	}

	function Bar() public view {
		// Feature: extracts invariant x < 33.
		assert(x != 33);
		assert(x != 34);
	}

	function Baz() public view {
		// Needs invariant x == y - 1.
		assert(x < y);
	}
}

