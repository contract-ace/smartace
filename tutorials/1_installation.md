---
layout: post
title: "1. Installing SmartACE"
date: 2020-05-18 15:00:00
categories: [smartace, install]
---

# 1. Installing SmartACE

SmartACE can be built from source, or obtained as a pre-built Docker container.

## Building from Source

SmartACE has the following dependencies:

  1. **CMake** version **3.0.0** or later,
  2. a **C++14** compliant compiler,
  3. **Boost** version **1.65** or later.

Afterwards. build SmartACE by running:

  1. `git clone https://github.com/ScottWe/solidity-to-cmodel.git`
  2. `cd solidity-to-cmodel ; git checkout cmodel-dev`
  3. `mkdir build ; cd build`
  4. `cmake .. -DCMAKE_INSTALL_PREFIX=run`
  5. `make install`

You should now have SmartACE installed as
`solidity-to-cmodel/build/run/bin/solc`.

### (Optional) Verify Your Installation

To verify your installation run `./scripts/soltest.sh --no-smt --no-ipc` from
the root directory.

### (Optional) Additional Dependencies

SmartACE supports many flavours of program analysis. Certain modes of analysis
have additional dependencies, as listed below:

  * **Model Checking**: the latest version of
    [Seahorn](https://github.com/seahorn/seahorn)
  * **Symbolic Execution**: the latest version of
    [Klee](https://github.com/klee/klee)
  * **Fuzzing**: a clang installation with
    [libfuzzer](https://llvm.org/docs/LibFuzzer.html) support

## Setup Through Docker

Coming soon!
