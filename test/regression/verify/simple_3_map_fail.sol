// RUN: %solc %s --c-model --map-k=2 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * This test compliments simple_3_map_pass. If the map is given insufficient
 * representatives, it should fail to validate the property. Otherwise, any pass
 * in simple_3_map_pass is spurious.
 */

contract ThreeMap {
	int a;
	int b;
	int c;
	mapping(int => int) map;

	constructor(int _a, int _b, int _c) public {
		a = _a;
		b = _b;
		c = _c;
		map[a] = _a;
		map[b] = _b;
		map[c] = _c;
	}

	function check() public view {
		assert(map[a] == a);
		assert(map[b] == b);
		assert(map[c] == c);
	}
}

