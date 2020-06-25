// RUN: %solc %s --c-model --lockstep-time=off --reps=1 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make icmodel

/*
 * A replication of basic.sol to ensure the --reps flag works.
 */

contract Contract {
	uint256 counter;
	function incr() public {
		counter = counter + 1;
		assert(counter < 2);
	}
}

