---
layout: post
title: "1. Installing SmartACE"
date: 2020-05-18 15:00:00
categories: [smartace, install]
---

# 1. Installing SmartACE

By Scott Wesley in collaboration Maria Christakis, Arie Gurfinkel, Xinwen Hu,
Jorge Navas, Richard Trefler, and Valentin Wüstholz.

SmartACE can be built from source, or obtained as a pre-built Docker container.
For end-users of SmartACE, we **strongly recommend** Docker as the
[choice of installation](#setup-through-docker).

Note that currently, SmartACE is locked at Solidity version `0.5.9`.

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
    [KLEE](https://github.com/klee/klee)
  * **Fuzzing**: a clang installation with
    [libfuzzer](https://llvm.org/docs/LibFuzzer.html) support

Under certain circumstances you may wish to edit a SmartACE model by hand. To do
this we highly recommend
[clang-format](https://clang.llvm.org/docs/ClangFormat.html).

## Setup Through Docker

[Docker](https://www.docker.com/) is a tool used by developers to share
installations and configurations of their applications in the form of *Docker
containers*. For those unfamiliar, Docker containers are isolated, portable
environment which share access to the operating system. If this is your first
time using Docker, start by installing the client for
[Ubuntu](https://docs.docker.com/installation/ubuntulinux/),
[OS X](https://docs.docker.com/installation/mac/), or
[Windows](https://docs.docker.com/installation/windows/).

Next, pull that latest version of SmartACE from DockerHub by running:

```
docker pull seahorn/smartace:arak
```

Now `cd` to the directory you wish to use SmartACE from. Run:

```
docker run -v $(pwd):/host -it seahorn/smartace:arak
```

This gives you terminal access to the SmartACE container. In this container,
SmartACE and all of its dependencies are added to your `PATH` variable. The
directory you were in prior to running Docker is mounted as `/host`.
