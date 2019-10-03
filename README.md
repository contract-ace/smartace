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

## Generating and Testing a Model

After making the project, you should find `./build/solc/socl`.
This is a modified version of the solidity compiler.
To generate a model, run `<PATH_TO_SOLC> <SRC1> [SRC2] ... [SRCn] --c-model --output-dir=<A_FRESH_DIRECTORY>`.
This will populate a CMake project.

Currently, integers provided from cstdint, and boost's multiprecision integers are supported.
To select one of these models pass `-DINT_MODEL:STRING=USE_STDINT` (resp. `-DINT_MODEL:STRING=USE_BOOST_MP`).
The model also expects the directory in which seahorn lives, given as `-DSEA_PATH=<SEAHORN_DIR>`.
Additionally, you may pass `-DSEA_ARGS=arg1;arg2;...`.
For instance, to compile a bit-precise counter-example, run `-DSEA_ARGS=--cex=cex.ll;--bv-cex`.

After running `cmake`, you may then generate an interactive model by running `make icmodel`.
You may invoke seahorn by running `make verify`.

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

## Generating Abstract Syntax Trees

The Solidity compiler can be configured to output JSON-formatted AST's.
This tool proves useful when debugging end-to-end issues.
The following command will consume a source unit through STDIN, and dump the AST to STDOUT.
```
solc --ast -
```

## More Information

Comprehensive documentation for solidity compiler development may be found in the [Developer's Guide](https://solidity.readthedocs.io/en/latest/contributing.html)

The original README.md for solidity has be renamed to solidity.md.
This file provides useful information on other aspects of the solidity project.
