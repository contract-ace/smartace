// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * This program is safe if it is impossible to send to a contract whose fallback
 * function is not payable.
 */

 contract A {
     address payable owner;
     constructor() public {
         owner = msg.sender;
     }
     function f() public payable {
         owner.transfer(msg.value);
     }
 }

 contract B {
     A a;
     constructor() public {
         a = new A();
     }
 }
