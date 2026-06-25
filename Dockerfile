FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Toolchain: gcc-13 (24.04 default), cmake, git.
# OpenMP ships with gcc (libgomp); BLS12-381 path in mcl needs no GMP.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Project sources (CMakeLists, include/, src/). .dockerignore drops build/ and mcl/.
COPY . /app

# Vendored mcl: add_subdirectory(mcl) expects ./mcl. Cloned at build time so it
# never has to live in your tree. Pin a tag here instead of master if you want
# a fully reproducible build.
RUN git clone --depth 1 https://github.com/herumi/mcl /app/mcl

# Configure + build (compiles mcl incl. the IFMA msm_avx unit, then the project).
RUN cmake -S /app -B /app/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /app/build -j"$(nproc)"