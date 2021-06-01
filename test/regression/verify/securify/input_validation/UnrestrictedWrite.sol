// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

contract A {
    address owner;
    function transferOwnership(address _newOwner) public {
        // INSTRUMENTATION (INITIALIZE FLAGS)
        bool _checked_newOwner = false;
        // INSTRUMENTATION (END)
        // INSTRUMENTATION (CHECK BEFORE WRITE)
        assert(_checked_newOwner);
        owner = _newOwner;
        // INSTRUMENTATION (END)
    }
}
