// RUN: %solc %s --c-model --output-dir=%t --bundle A --invar-type rolebased
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * If a role-based invariant is used, then the role fields will persist across
 * transactions. In this contract, role-related fields are set to 5 after the
 * role changes. Between role changes, only users without roles may change their
 * fields. Therefore, all role-related data should always be 5.
 */

contract A {
    mapping(address => int) m1;
    mapping(address => mapping(address => int)) m2;

    address role_a;
    address role_b;

    constructor() public {
        _reset();
    }

    function set_role_a() public {
        role_a = msg.sender;
        _reset();
    }

    function set_role_b() public {
        role_b = msg.sender;
        _reset();
    }

    function inc_m1() public {
        require(msg.sender != role_a);
        require(msg.sender != role_b);
        m1[msg.sender] = m1[msg.sender] + 1;
    }

    function inc_m2(address o) public {
        require(msg.sender != role_a);
        require(msg.sender != role_b);
        require(o != role_a);
        require(o != role_b);
        m2[msg.sender][o] = m2[msg.sender][o] + 1;
    }

    function check() public {
        assert(m1[role_a] == 5);
        assert(m1[role_b] == 5);
        assert(m2[role_a][role_a] == 5);
        assert(m2[role_a][role_b] == 5);
        assert(m2[role_b][role_a] == 5);
        assert(m2[role_b][role_b] == 5);
    }

    function _reset() internal {
        m1[role_a] = 5;
        m1[role_b] = 5;
        m2[role_a][role_a] = 5;
        m2[role_a][role_b] = 5;
        m2[role_b][role_a] = 5;
        m2[role_b][role_b] = 5;
    }
}
