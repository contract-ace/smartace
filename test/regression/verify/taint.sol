// RUN: %solc %s --c-model --concrete --reps=1 --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * An alternative approach to model-checking is static analysis. In PLDI2020, a
 * paper was published on detecting "composite (security) vulnerabilities" in
 * Solidity programs. This is a class of dataflow properties which may require
 * an arbitrary number of actions to violate. This contract translates one of
 * their motivating examples into a bounded model checking problem, to ensure
 * that SmartACE can handle these properties.
 */

/**
 * Adapted from "Ethainter: A Smart Contract Security Analyzer for Composite
 * Vulnerabilities" by Brent, L., Grech, N., Lagouvardos, S., Scholz, B., and
 * Smaragdakis, Y., in PLDI2020.
 */
contract Victim {
	address owner;
	mapping(address => bool) admins;
	mapping(address => bool) users;

	modifier onlyAdmins() {
		require(admins[msg.sender]);
		_;
	}

	modifier onlyUsers() {
		require(users[msg.sender]);
		_;
	}

	function register() public {
		users[msg.sender] = true;
	}

	function referUser(address user) public onlyUsers {
		users[user] = true;
	}

	function referAdmin(address adm) public onlyUsers {
		admins[adm] = true;
	}

	function changeOwner(address o) public onlyAdmins {
		owner = o;
	}

	function kill() public view onlyAdmins {
		// selfdestruct(owner);
		assert(owner == msg.sender);
	}
}

