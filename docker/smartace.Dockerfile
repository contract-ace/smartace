#
# Minimal Dockerfile for SmartACE
# Produces a lightweight container with Seahorn and SmartACE.
# TODO: Klee support.
#

FROM seahorn/seahorn-llvm10:nightly
USER root

WORKDIR /smartace
RUN git clone https://github.com/ScottWe/solidity-to-cmodel.git /smartace

WORKDIR /smartace/build
RUN git checkout cmodel-dev
RUN cmake .. -DCMAKE_INSTALL_PREFIX=run
RUN make install -j8

RUN useradd -ms /bin/bash smartace
WORKDIR /home/smartace
USER smartace
ENV PATH="$PATH:/smartace/build/run/bin"

