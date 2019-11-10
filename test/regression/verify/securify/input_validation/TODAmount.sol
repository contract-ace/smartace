// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cexcmodel | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

/**
 * Note that this contract did not provide a way to set owner. Securify's static
 * analysis overapproximates behaviour enough for this to be a non-issue. In
 * reality, owner is always the NULL address, and as such, the unvalidated write
 * is dead code To fix this, I have added a constructor which sets owner.
 */

contract A {
    mapping(address => uint) balances;
    uint price = 10;
    address owner;
    constructor() public {
        owner = msg.sender;
    }
    function setPrice(uint newPrice) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_newPrice = false;
        // INSTRUMENTATION (END)
        if (msg.sender == owner)
        // INSTRUMENTATION (CHECK BEFORE WRITE)
        assert(_checked_newPrice);
        price = newPrice;
        // INSTRUMENTATION (END)
    }
    function sellTokens() public {
        uint amount = balances[msg.sender];
        balances[msg.sender] = 0;
        msg.sender.transfer(amount * price);
    }
}
