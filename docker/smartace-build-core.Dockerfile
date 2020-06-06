#
# Build Dockerfile for SmartACE
# This produces the SmartACE distribution at /smartace/build/.
# To extract, run:
#     docker run -v $(pwd):/host -it [IMG] /bin/sh -c "cp *.tar.gz /host/"
#

FROM buildpack-deps:bionic
USER root

RUN apt-get update && \
    apt-get install -yqq software-properties-common && \
    apt-get update && \
    apt-get upgrade -yqq && \
    apt-get install -yqq build-essential g++ gcc cmake git && \
    update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.gold" 20 && \
    update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.bfd" 10 && \
    apt-get install -yqq libboost-all-dev && \
    git clone https://github.com/ScottWe/solidity-to-cmodel.git /smartace

WORKDIR /smartace/build
RUN cmake ../ \
        -DUSE_CVC4=OFF \
        -DUSE_Z3=OFF \
        -DCPACK_GENERATOR="TGZ" \
        -DCMAKE_INSTALL_PREFIX=/home/usea/smartace && \
    make package -j8
