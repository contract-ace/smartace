// RUN: %solc %s --c-model --output-dir=%t --bundle CryptoTest
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * There must exist x and y such that keccak256(_x) == keccak256(_y).
 */

contract CryptoTest {
	function f(bytes32 _x, bytes32 _y) public pure {
		bytes32 x = keccak256(abi.encodePacked(_x));
        bytes32 y = keccak256(abi.encodePacked(_y));
        assert(x != y);
	}
}
