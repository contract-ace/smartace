// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This contract is only safe if modifiers are handled correctly.
 */

contract ModifierContract {
	int x;
	int y;

	modifier TestMod(int _x, int _y) {
		require(_x + x >= 0);
		require(_x + _x + x >= 0);
		require(_y <= 0);
		_;
		_;
	}

	function Op(int _x, int _y) public TestMod(_x, _y) {
		x = x + _x;
		y = _y;
		assert(x >= 0);
		assert(y <= 0);
	}
}
