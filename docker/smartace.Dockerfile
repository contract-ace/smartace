#
# Minimal Dockerfile for SmartACE
# Produces a lightweight container with Seahorn, Klee, and SmartACE.
#

FROM seahorn/seahorn-llvm10:nightly

USER usea
COPY smartace-*.tar.gz /tmp
COPY klee.tar.gz /tmp
WORKDIR /home/usea
RUN mkdir -p /home/usea/smartace && \
    mkdir -p /home/usea/klee && \
    tar xf /tmp/smartace-*.tar.gz -C smartace --strip-components=1 && \
    tar xf /tmp/klee.tar.gz -C klee --strip-components=3 && \
    git clone https://github.com/ScottWe/smartace-examples.git /home/usea/examples


USER root
RUN rm -rf /tmp/smartace-*.tar.gz && \
    rm -rf /tmp/klee.tar.gz

USER usea
ENV PATH="$PATH:/home/usea/klee/bin:/home/usea/smartace/bin"
