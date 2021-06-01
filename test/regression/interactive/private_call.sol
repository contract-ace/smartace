// RUN: %solc %s --reps=1 --lockstep-time=off --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath %buildargs
// RUN: make icmodel
// RUN: echo 0 0 2 1 0 0 2 0 | ./icmodel --return-0 --count-transactions 2>&1 | OutputCheck %s --comment=//
// CHECK: assert
// CHECK: Transaction Count: 1

/*
 * This regression test ensures private calls work in interactive.
 */

contract Simple {
	function private_call(int a) private {
		assert(a != 0);
	}

	function public_call(int a) public {
		private_call(a);
	}
}

