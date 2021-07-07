// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type singleton --invar-rule checked --invar-infer on
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Checks that invariants are generated for structures in mappings.
 * This property requries 100 users, OR an invariant.
 */

library Roles {
    struct Role {
        mapping (address => bool) bearer;
    }
}

contract MinterRole {
    using Roles for Roles.Role;
    Roles.Role private _minters;
}

contract Token is MinterRole {
    function f() public {}
}
