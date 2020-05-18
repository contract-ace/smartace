// RUN: %solc %s --c-model --reps=3 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Checks that the map maintains constants and contracts.
 */

contract ThreeMap {
	mapping(address => uint256) map;
	function check() public view {
		assert(map[address(10)] == 0);
		assert(map[address(this)] == 0);
	}
}

