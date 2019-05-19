# Model Checking in Solidity

Extension of the solidity compiler to support smart contract model checking.

## Repository Overview

Code related to code generation, and the solidity AST, may be found in `libsolidity`.
These components are integrated within `solc/` to build `./solc`, the solidity compiler.
Tests for these libraries may be found under `test/libsolidity`.
The libsolidity testsuite is accessible for `scripts/solctests.sh`.

## Building the Project

Before building `solc`, ensure that the latest version of cmake is installed.
To compile `solc` for the first time, run
```
mkdir build
cd build
cmake ..
```
Once cmake has finished, simply run `make` from within `build/`.

If needed, refer to the [official build documentation](https://solidity.readthedocs.io/en/latest/installing-solidity.html#building-from-source)

## Adding New Modules and Tests

To add a new file to `libsolidity/`, its path must be added to `libsolidity/CMakeLists.txt`.
For each module added, some test cases should also be introduced.
Unlike the libraries, the test CMake files need only be updated if a new directory is added.

## Testing the Solidity Compiler

The libsolidity testsuite is built on top of Boost's unit testing framework.
A simply testsuite will be of the form,
```
BOOST_AUTO_TEST_SUITE(SuiteName);

...

BOOST_AUTO_TEST_CASE(test_name)
{
    ...
}

...

BOOST_AUTO_TEST_SUITE_END()
```

It is recommended that test cases use `test/libsolidity/AnalysisFramework.h`.
This provides a test fixture which allows solidity to AST compiling within a testcase.
By using solidity code within the test, the scope of each test becomes more evident.

The script `scripts/solctests.sh` will execute tests only from libsolidity.
The test script will use the latest version of `solctest` found under `build/test`.
By default, all tests related to Z3 and aneth will be executed.
These are not needed in this project, and may be bypassed with the following command.
```
/script/solctests.sh --no-smt --no-ipc
```

For more information on the libsolidity testsuite, or to learn how to test other modules, refer to [Running the compiler tests](https://solidity.readthedocs.io/en/latest/contributing.html#running-the-compiler-tests) from the official developer documents.

## More Information

Comprehensive documentation for solidity compiler development may be found in the [Developer's Guide](https://solidity.readthedocs.io/en/latest/contributing.html)

The original README.md for solidity has be renamed to solidity.md.
This file provides useful information on other aspects of the solidity project.
