// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * This model requires that DerivedContract is assignable to BaseContract. As
 * the model specializes contracts, this requires knowledge that c is equivalent
 * to DerivedContract.
 */

contract BaseContract {
	int a;
	constructor(int _a) public { a = _a; }
}

contract DerivedContract is BaseContract {
	constructor() public BaseContract(5) {}
	function g() public view returns (int) { return a; }
}

contract User {
	BaseContract c;
	constructor() public { c = new DerivedContract(); }
}
