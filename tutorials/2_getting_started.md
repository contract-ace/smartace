---
layout: post
title: "2. Finding Representation Invariants for Smart Contracts"
subtitle: "Basic Auditing with SmartACE"
date: 2020-05-18 15:00:00
categories: [smartace, verification, model checking, fuzzing, invariants]
---

# 2. Finding Representation Invariants for Smart Contracts

By Scott Wesley in collaboration Maria Christakis, Arie Gurfinkel, Xinwen Hu,
Jorge Navas, and Valentin Wüstholz.

An important aspect of smart contract analysis is functional verification. That
is, the ability to check if a *bundle* (a set of smart contracts) satisfies a
given requirement. Often these requirements are written independently from the
implementation and refer only to the interface.

In this tutorial we will prove a simple requirement by finding a "representation
invariant", and then use fuzz testing to corroborate our proof. We will learn
to:

  1. Instrument a smart contract at the Solidity level.
  2. Generate a SmartACE model from a single smart contract.
  2. Use software model checking to find a representation invariant.
  4. Use fuzz testing to search for violations in the smart contract.

**Note**: This tutorial assumes all commands are run from within the
[SmartAce container](1_installation.md). All tutorial files are available within
the container from the home directory.

## Instrumenting a Solidity Smart Contract

Let's start with a *crowdsale* smart contract designed to raise `goal` funds by
`deadline`. Clients may call `finish()` if the goal is reached by `deadline`,
and `cancel()` if the goal is not reached by `deadline`.

```solidity
contract Crowdsale {
    uint raised;
    uint goal;
    uint deadline;

    constructor(uint _goal) public {
        goal = _goal;
        deadline = now + 365 days;
    }

    function invest() public payable {
        require(now <= deadline);
        raised += msg.value;
    }

    function finish() public {
        require(address(this).balance >= goal);
    }

    function cancel() public {
        require(address(this).balance < goal && now > deadline);
    }
}
```

A real crowdsale would track each client's contribution through the use of a
map. We will look at map properties in a future tutorial. For now, let's verify
that `finish()` and `cancel()` are mutually exclusive.

Our property says that given a valid implementation, `Crowdsale` should never
move between a *finished* state and *canceled* state. We prove this property by
finding a [representation invariant](http://www.cs.cornell.edu/courses/cs312/2005sp/lectures/lec09.html).
The invariant will summarize the concrete states of `Crowdsale`, and imply that
*finished* and *canceled* are mutually exclusive.

We can instrument this by:

  1. Adding two ghost variables which track when the contract is `finished` or
     `canceled`.
  2. Adding a public method which asserts that both ghost variables are never
     true at the same time.

The instrumented contract is given below. In later tutorials we will learn to
mechanically instrument smart contracts from temporal specification. For now,
save a copy of the
[instrumented smart contract](https://github.com/ScottWe/smartace-examples/blob/master/tutorials/post-2/crowdsale.sol).

```solidity
contract Crowdsale {
    uint raised;
    uint goal;
    uint deadline;

    bool finished; // Instrumented.
    bool canceled; // Instrumented.

    constructor(uint _goal) public {
        goal = _goal;
        deadline = now + 365 days;
    }

    function invest() public payable {
        require(now <= deadline);
        raised += msg.value;
    }

    function finish() public {
        require(address(this).balance >= goal);
        finished = true; // Instrumented.
    }

    function cancel() public {
        require(address(this).balance < goal && now > deadline);
        canceled = true; // Instrumented.
    }

    function repOK() public view {
        assert(!(finished && canceled)); // Instrumented.
    }
}
```

## Generating a SmartACE Model

You can generate a model checking friendly model by running:

```
solc crowdsale.sol --bundle=Crowdsale --c-model --output-dir=mc
```

You can generate a fuzzer friendly model by running:

```
solc crowdsale.sol --bundle=Crowdsale --concrete --reps=5 --c-model \
    --output-dir=fuzz
```

A pre-generated snapshot of the output is
[also available](https://github.com/ScottWe/smartace-examples/blob/master/tutorials/post-2/).

In future tutorials we will look at the model encoding. For now we focus on the
parameters to the tool.

  * `bundle` is the list of smart contracts we want to verify.
  * `concrete` requests a model with a bounded number of clients.
  * `reps` gives the number of concrete clients ("*representatives*") to
    generate.
  * `c-model` sets the output mode to a SmartACE model.
  * `output-dir` gives the output directory.

To view all options execute `/path/to/install/bin/solc --help`.

## Checking the SmartACE Model

[Model checkers](https://arieg.bitbucket.io/pdf/ModelChecking.pdf) can be used
to validate that a logical representation of a program satisfies some problem.
This comes at the cost of some abstractions, which we will explore in future
tutorials. To run the model checker:

  1. `cd mc ; mkdir build ; cd build`
  2. `CC=clang-10 CXX=clang++-10 cmake ..`
  3. `cmake --build . --target  verify`

This will use `Seahorn` to verify the smart contract for an unbounded number of
clients. The default analysis handles integers arithmetically. That is, overflow
and underflow are not modeled. When the analysis completes, you will see a
certificate of correctness. This certificate is the representation invariant.

In future tutorials, we will learn how to interpret this certificate, and how to
debug a model when `Seahorn` fails to verify the property.

## Fuzzing the SmartACE Model

[Fuzz testing](https://llvm.org/docs/LibFuzzer.html) is a form of automated bug
hunting. Fuzz testing works by generating random inputs, in the hopes of
violating an assertion. Fuzz tests do not require abstractions, but are limited
in their coverage. To run the fuzzer:

  1. `cd fuzz ; mkdir build ; cd build`
  2. `CC=clang-10 CXX=clang++-10 cmake ..`
  3. `cmake --build . --target fuzz`

By default, `fuzz` will run `1000000` trials with a timeout of `15` seconds.
Each trial terminates if a `require` fails. A trial fails if an `assert` fails.
After a few minutes, all trial will pass, as expected. You will see a summary
of the tests printed to the console.

## Conclusion

In this tutorial we learned how to use SmartACE to analyze smart contracts with
assertions. We saw how supporting a spectrum of analysis techniques, such as
model checking and fuzz testing, can enable different forms of model validation.
In the next tutorial, we will look closer at model checking in SmartACE.
