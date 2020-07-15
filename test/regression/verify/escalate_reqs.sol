// RUN: %solc %s --c-model --fail-on-require --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

contract EscalatesTest {
    uint256 count;
	function f() public {
        count = count + 1;
        require(count < 10);
	}
}
