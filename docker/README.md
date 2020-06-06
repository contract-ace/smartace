# Docker Builds for SmartACE

This directory containerizes the distribution of SmartACE. The containers
`smartace-build-*.Dockerfile` are used to build packages from source. These
packages are extracted, and then consumed by `smartace.Dockerfile`.

## Build Instructions

First install [Docker](https://hub.docker.com/). Afterwards, you can build the
SmartACE Docker environment by running:

```
docker build --tag smartace-builder-klee -f /path/to/smartace/docker/smartace-build-klee.Dockerfile .
docker build --tag smartace-builder-core -f /path/to/smartace/docker/smartace-build-core.Dockerfile .
docker run -v $(pwd):/host -it smartace-builder-klee /bin/sh -c "cp *.tar.gz /host/"
docker run -v $(pwd):/host -it smartace-builder-core /bin/sh -c "cp *.tar.gz /host/"
docker build --tag smartace/smartace:arak -f /path/to/smartace/docker/smartace.Dockerfile .
```
