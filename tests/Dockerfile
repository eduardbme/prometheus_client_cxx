FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        curl \
        gcc-14 g++-14 \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /third_party

COPY third_party/googletest/ ./googletest

RUN mkdir -p googletest/build && cd googletest/build && \
    cmake ../ \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_CXX_STANDARD=17 \
          -DCMAKE_INSTALL_PREFIX=. && \
    make -j$(nproc) && \
    make install

WORKDIR /app

COPY single_include/ ./single_include/
COPY tests/ ./tests/

WORKDIR /app/tests

ENV GTest_DIR=/third_party/googletest/build/lib/cmake/GTest

RUN cmake -B build -S . && \
    cmake --build build

CMD ["sh", "-c", "ctest --test-dir build --output-on-failure && ./build/runTsan"]
