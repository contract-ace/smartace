# Securify Tests

## Introduction

The Securify test suite exercises Securify against simple implementations. These
implementations exercise the space of properties checked by Securify. In this
directory, (at least one) equivalent test case defined for each Securify test
case. These test cases have been hand instrumented to capture the violations
which Securify finds through static analysis. It is not guarenteed that these
encodings our optimal, simple that our tool could potentially match the features
of Securify.

## Major Changes

1. reentrancy2.sol was heavily transformed to match the features we support.

## Globally Omitted Tests

Certain test cases depend on the behaviour of instructions only accessible
through assembly. These are outside the scope of our tool. Such tests have been
listed below.

* TestShl.sol
* TestShr.sol
* TestCreate2Factory.sol
* TestExtcodeHash.sol

## License

The original test suite was licensed under the Apache 2.0 license. As required
by said license, the license file has been reproduced with these modified tests.
The original tests were distributed by SRI Labs, and are available through
https://github.com/eth-sri/securify.
