#
# Build Dockerfile for Klee
# This produces the Klee distribution at /klee/build/.
# To extract, run:
#     docker run -v $(pwd):/host -it [IMG] /bin/sh -c "cp *.tar.gz /host/"
#

FROM buildpack-deps:bionic
USER root

WORKDIR /tmp
RUN apt-get update && \
    apt-get install -yqq software-properties-common && \
    apt-get update && \
    apt-get upgrade -yqq && \
    apt-get install -yqq build-essential g++ gcc cmake git && \
    update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.gold" 20 && \
    update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.bfd" 10 && \
    apt-get install -yqq libgoogle-perftools-dev libsqlite3-dev && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 10 && \
    useradd -m usea && \
    git clone https://github.com/klee/klee.git /klee

WORKDIR /klee/build
RUN mkdir - p /home/usea/klee && \
    cmake ../ \
        -DENABLE_POSIX_RUNTIME=OFF \
        -DENABLE_KLEE_UCLIBC=OFF \
        -DENABLE_UNIT_TESTS=OFF \
        -DENABLE_SYSTEM_TESTS=OFF \
        -DENABLE_TCMALLOC=OFF \
        -DENABLE_SOLVER_STP=OFF \
        -DENABLE_SOLVER_Z3=ON \
        -DCMAKE_INSTALL_PREFIX=/home/usea/klee \
        -DLLVM_CONFIG_BINARY=/usr/bin/llvm-config-10 \
        -DLLVMCC=/usr/bin/clang-10 \
        -DLLVMCXX=/usr/bin/clang++-10 && \
    make install -j8 && \
    tar -czvf klee.tar.gz /home/usea/klee
