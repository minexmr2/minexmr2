FROM alpine:3.16 AS builder

RUN apk add --no-cache git gcc g++ cmake make libtool

ARG ref=origin/master
ARG repo=https://github.com/SChernykh/p2pool

RUN git clone ${repo} /src/p2pool
WORKDIR /src/p2pool
RUN git reset --hard ${ref} && git submodule sync && git submodule update --init --recursive

WORKDIR /src/p2pool/external/src/libzmq
RUN mkdir build && cd build && \
    cmake .. -DENABLE_WS=OFF -DWITH_TLS=OFF -DENABLE_CURVE=OFF && \
    make -j$(nproc)

WORKDIR /src/p2pool/external/src/libuv
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

WORKDIR /src/p2pool

RUN mkdir build && cd build && \
    CFLAGS="-march=native -mtune=native -Ofast -Wno-error=unused-variable" CXXFLAGS="-march=native -mtune=native -Ofast -Wno-error=unused-variable" \
    cmake .. -DSTATIC_BINARY=ON && \
    make -j$(nproc)

FROM alpine:3.16

RUN apk add --no-cache curl jq bash

RUN mkdir -p /p2pool

COPY --from=builder /src/p2pool/build/p2pool /p2pool/p2pool

EXPOSE 3333 37889

WORKDIR /p2pool/data

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
