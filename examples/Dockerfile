FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        curl \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY single_include/ ./single_include/
COPY examples/ ./examples/

WORKDIR /app/examples

RUN cmake -B build -S . && \
    cmake --build build

CMD ["/bin/bash"]
