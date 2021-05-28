// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

contract A {
    mapping(address => uint) balances;
    function withdraw(uint amount) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_amount = false;
        // INSTRUMENTATION (END)
        // INSTRUMENTATION (CHECK BEFORE WRITE)
        assert(_checked_amount);
        balances[msg.sender] -= amount;
        // INSTRUMENTATION (END)
        msg.sender.transfer(amount);
    }
}
