# Input Validation

## Explanation

Input validation checks that a user input is checked before writing its value to
storage. I have approximated this by creating a flag for each variable. If the
variable is read as part of a conditional statement, the flag is raised. If the
variable appears in an assignment statement, it is asserted to be true.

## Thoughts on Implementation

In most cases, this is trivial. We need only track a flag for each variable, and
raise it when a branch depends on said variable. As public methods may be called
from any address, we do not need to distinguish between internal calls to public
methods and external calls to public methods.

However, consider the case where f is internal. Then if g calls f, we must know
if the arguments f are tainted, and if so, if they have been sanitized. A
solution is as follows.

1. Move the flags for all internal members to the contract's state.
2. Before calling an internal member, set its flags to reflect sanitation
3. Reset the flags of said internal member after returning, in case of recursion

Thus, if some argument to f is never tainted, or always sanitized, f may safely
elide sanitation checks without the risk of false alarms. An example follows.

```
contract A {
    // Instrumentaiton: bool _checked_f_arg1;
    // Instrumentaiton: bool _checked_f_arg2;
    int _b;
    function f(int arg1, int arg2) internal {
        // [Checks arg1 and arg2 are sanitized]
        //   Instrumentaiton: require(_checked_f_arg1 && checked_f_arg2);
        _b = arg1 + arg2;
    }
    function g(int a) public {
        // [Placeholders for calls, to avoid redeclaration]
        //   Instrumentation: bool _tmpchecked_f_arg1;
        //   Instrumentation: bool _tmpchecked_f_arg2;
        // [Value a is initially unchecked]
        //   Instrumentation: bool _checked_a = false;
        // [Value a is checked by following conditional]
        //   Instrumentation: _checked_a = true;
        if (a == 42) {
            revert();
        }
        // [State is saved before entering internal member]
        //   Instrumentation: _tmpchecked_f_arg1 = _checked_f_arg1;
        //   Instrumentation: _tmpchecked_f_args = _checked_f_arg2;
        // [State is updated for this call]
        //   Instrumentation: _checked_f_arg1 = _checked_a;
        //   Instrumentation: _checked_f_arg2 = true;
        f(a, _b); // `arg1` has been sanitized while `arg2` is not tainted
        // [State is reverted]
        //   Instrumentation: _checked_f_arg1 = _tmpchecked_f_arg1;
        //   Instrumentation: _checked_f_arg2 = _tmpchecked_f_arg2;
    }
}
```

Some cases would require true taint analysis. Such an example is given below.
```
contract A {
    int _b;
    function f(int a) public {
        int b = a;
        int _b = b; // VIOLATION: b is tainted by a
    }
    function g(int a) public {
        int b = a;
        if (b == 42) {
            revert();
        }
        _b = a; // NOT A VIOLATION: b was tainted by a, and b was validated
    }
}
```

All of this can be performed on the IR transformation, to achieve an optimal
encoding.

## Omitted Tests

In test caes without inputs, this is trivial, given that we have hand-crafted
the instrumentation. The same can be said for tests in which user input never
touches global state. Such tests have been omitted. (Note that these tests are
interesting should we automate the instrumentation, they are trivially safe).

The forementioned circumstances reduce to unused flag vairables, or instances of
`require(true)`, or some combination of both.

* DAO.sol
* DAOConstantGas.sol
* locked-money-no-self-destruct-no-call-unsafe.sol (only interesting as bytecode)
* locked-money-call-safe.sol
* locked-money-call-ten-safe.sol
* locked-money-call-zero-unsafe.sol
* locked-money-self-destruct-safe.sol
* LockedEther.sol (only interesting as bytecode)
* LockedEtherDelegate.sol
* no-reentrancy.sol
* reentrancy.sol
* repeated-calls-tn.sol
* repeated-calls-tn2.sol
* repeated-calls-tn3.sol
* repeated-calls-tp.sol
* transaction-reordering.sol
* TODTransfer2.sol
* UnhandledException.sol
* UnhandledException2.sol
* UnrestrictedEtherFlow.sol
* UnrestrictedEtherFlow2.sol
