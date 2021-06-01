// RUN: %solc %s --c-model --lockstep-time=off --aux-users=1 --output-dir=%t --bundle Contract
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel

/*
 * A replication of basic.sol to ensure the --aux-users flag works.
 */

contract Contract {
	uint256 counter;
	function incr() public {
		counter = counter + 1;
		assert(counter < 2);
	}
}

