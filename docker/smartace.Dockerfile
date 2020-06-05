#
# Minimal Dockerfile for SmartACE
# Produces a lightweight container with Seahorn and SmartACE.
# TODO: Klee support.
#

FROM seahorn/seahorn-llvm10:nightly
USER root

RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt-get update
RUN sudo apt-get install -y cmake

WORKDIR /smartace
RUN git clone https://github.com/ScottWe/solidity-to-cmodel.git /smartace

WORKDIR /smartace/build
RUN git checkout cmodel-dev
RUN cmake .. -DCMAKE_INSTALL_PREFIX=run
RUN make install -j8

RUN useradd -ms /bin/bash usmart
RUN echo usmart:ace | chpasswd
RUN usermod -aG sudo usmart
WORKDIR /home/usmart
USER usmart
ENV PATH="$PATH:/smartace/build/run/bin"

