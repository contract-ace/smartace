// RUN: %solc %s --c-model --map-k=3 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Checks that the contents of three elements may be reasoned about
 * deterministically, given a large enough local space for the map. Functional
 * tests already exist for the mapping model, so this test is concerned with the
 * verification engine's ability to reason about it.
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

